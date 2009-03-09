// Copyright 2008 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz
// or its licensors, as applicable.
// 
// You may not use this file except under the terms of the accompanying license.
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you
// may not use this file except in compliance with the License. You may
// obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 
// Project: ocrofst
// File: fst-util.cc
// Purpose:
// Responsible: mezhirov
// Reviewer: 
// Primary Repository: 
// Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org

#include "ocr-pfst.h"

using namespace colib;
using namespace ocropus;

namespace {
    // Pick an array element with probability proportional to exp(-cost).
    static int sample_by_costs(floatarray &costs) {
        using namespace narray_ops;
        doublearray p;
        copy(p, costs);
        p -= min(costs);
        
        for(int i = 0; i < p.length(); i++)
            p[i] = exp(-p[i]);
        p /= sum(p);
        
        double choice = double(rand()) / RAND_MAX;
        double s = 0;
        for(int i = 0; i < p.length(); i++) {
            s += p[i];
            if(choice < s)
                return i;
        }

        // shouldn't happen...
        return costs.length() - 1;
    }
}

namespace ocropus {
    void remove_epsilons(nustring &s, intarray &a) {
        s.clear();
        for(int i = 0; i < a.length(); i++) {
            if(a[i])
                s.push(nuchar(a[i]));
        }
    }

    double fst_sample(intarray &result, IGenericFst &fst, int max) {
        double total_cost = 0;
        int current = fst.getStart();

        for(int counter = 0; counter < max; counter++) {
            intarray inputs;
            intarray outputs;
            intarray targets;
            floatarray costs;

            fst.arcs(inputs, targets, outputs, costs, current);

            // now we need to deal with the costs uniformly, so:
            costs.push(fst.getAcceptCost(current));
            int choice = sample_by_costs(costs);
            if(choice == costs.length() - 1)
                break;
            result.push(outputs[choice]);
            total_cost += costs[choice];
            current = targets[choice];
        }
        return total_cost + fst.getAcceptCost(current);
    }


    double fst_sample(nustring &result, IGenericFst &fst, int max) {
        intarray tmp;
        double cost = fst_sample(tmp, fst, max);
        remove_epsilons(result, tmp);
        return cost;
    }

    void fst_star(IGenericFst &fst) {
        int s = fst.getStart();
        fst.setAccept(s);
        for(int i = 0; i < fst.nStates(); i++) {
            double c = fst.getAcceptCost(i);
            if(c < 1e37)
                fst.addTransition(i, s, 0, c, 0);
        }
    }
    
    void fst_star(IGenericFst &result, IGenericFst &fst) {
        fst_copy(result, fst);
        fst_star(result);
    }

    /// Returns a set of numbers that occur in the input array.
    void get_alphabet(intarray &alphabet, intarray &symbols) {
        intarray c;
        copy(c, symbols);
        quicksort(c);
        alphabet.clear();
        if(!c.length())
            return;
        alphabet.push(c[0]);
        for(int i = 1; i < c.length(); i++) {
            if(c[i] != c[i - 1])
                alphabet.push(c[i]);
        }
    }

    static void append_to_intarray(intarray &result, intarray &string) {
        for(int i = 0; i < string.length(); i++)
            result.push(string[i]);
    }

    void get_alphabet(intarray &alphabet, intarray &sym1, intarray &sym2) {
        intarray t;
        copy(t, sym1);
        append_to_intarray(t, sym2);
        get_alphabet(alphabet, t);
    }

    static void append_to_intarray(intarray &result, nustring &string) {
        for(int i = 0; i < string.length(); i++)
            result.push(string[i].ord());
    }

    static void append_to_intarray(intarray &result, objlist<nustring> &strings) {
        for(int i = 0; i < strings.length(); i++)
            append_to_intarray(result, strings[i]);
    }

    void get_alphabet(intarray &alphabet, objlist<nustring> &dict) {
        intarray symbols;
        append_to_intarray(symbols, dict);
        get_alphabet(alphabet, symbols);
    }

    void get_alphabet(intarray &alphabet, IGenericFst &fst) {
        int n = fst.nStates();
        intarray all;
        for(int i = 0; i < n; i++) {
            intarray inputs;
            intarray outputs;
            intarray targets;
            floatarray costs;
            fst.arcs(inputs, targets, outputs, costs, i);
            append_to_intarray(all, inputs);
            append_to_intarray(all, outputs);
        }
        get_alphabet(alphabet, all);
    }

    void fst_line(IGenericFst &fst, nustring &s) {
        int n = s.length();
        intarray inputs(n);
        for(int j = 0; j < n; j++)
            inputs[j] = s[j].ord();
        floatarray costs(n);
        fill(costs, 0);
        fst.setString(s, costs, inputs);
    }

    void fst_insert_line(IGenericFst &fst, nustring &s, int start, int end) {
        if(s.length() == 0)
            fst.addTransition(start, end, 0, 0, 0);
        else {
            int last = start;
            for(int i = 0; i < s.length() - 1; i++) {
                int next = fst.newState();
                fst.addTransition(last, next, s[i].ord(), 0, s[i].ord());
            }
            int c = s[s.length() - 1].ord();
            fst.addTransition(last, end, c, 0, c);
        }
    }

    void fst_insert_line(IGenericFst &fst, const char *line, int s, int e) {
        nustring str(line);
        fst_insert_line(fst, str, s, e);
    }

    void fst_insert_bunch(IGenericFst &fst, intarray &s, int start, int end) {
        for(int i = 0; i < s.length(); i++)
            fst.addTransition(start, end, s[i], 0, s[i]); 
    }

    void fst_insert_bunch(IGenericFst &fst, nustring &s, int start, int end) {
        for(int i = 0; i < s.length(); i++)
            fst.addTransition(start, end, s[i].ord(), 0, s[i].ord()); 
    }

    void fst_insert_bunch(IGenericFst &fst, const char *s, int start, int end) {
        nustring str(s);
        fst_insert_bunch(fst, str, start, end);
    }
}
