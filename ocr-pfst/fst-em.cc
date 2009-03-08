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
// Project:
// File: fst-em.cc
// Purpose: EM training of FSTs
// Responsible: mezhirov
// Reviewer:
// Primary Repository:
// Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org

#ifndef fst_em_h_
#define fst_em_h_

#include "ocropus.h"
#include "langmods.h"

using namespace colib;
using namespace ocropus;

namespace {

    /// We'll use the archash to store best costs.
    /// The key is either the input or the output of some arc,
    /// and the value is either -1 meaning that no arcs are present
    /// or positive for the best cost.
    typedef inthash< Floating<-1> > archash;

    void fill_archash(archash &result, IGenericFst &fst,
                      int from, int to, int symbol, bool symbol_is_input) {
        intarray inputs;
        intarray targets;
        intarray outputs;
        floatarray costs;
        fst.arcs(inputs, targets, outputs, costs, from);
        for(int i = 0; i < targets.length(); i++) {
            if(to != targets[i])
                continue;
            if(symbol != (symbol_is_input ? inputs[i] : outputs[i]))
                continue;

            result((symbol_is_input ? outputs[i] : inputs[i])) = i;
        }
    }

    struct Arc {
        int to;
        int input;
        int output;
        float cost;
        int counter;
    };

    struct StandardTrainableFst: ITrainableFst {
        colib::objlist< colib::narray<Arc> > arcs_; // we have a method arcs()
        colib::floatarray accept_costs;
        int start;

        StandardTrainableFst() : start(0) {}

        const char *name() {
            return "standardtrainablefst";
        }
        virtual const char *description() {
            return "StandardTrainableFst";
        }

        // reading
        virtual int nStates() {
            return arcs_.length();
        }
        virtual int getStart() {
            return start;
        }
        virtual float getAcceptCost(int node) {
            return accept_costs[node];
        }
        virtual void arcs(colib::intarray &ids,
                          colib::intarray &targets,
                          colib::intarray &outputs,
                          colib::floatarray &costs,
                          int from) {
            narray<Arc> &a = arcs_[from];
            makelike(ids, a);
            makelike(targets, a);
            makelike(outputs, a);
            makelike(costs, a);
            for(int i = 0; i < a.length(); i++) {
                ids[i]     = a[i].input;
                targets[i] = a[i].to;
                outputs[i] = a[i].output;
                costs[i]   = a[i].cost;
            }
        }
        virtual void clear() {
            arcs_.clear();
            accept_costs.clear();
            start = 0;
        }

        // writing
        virtual int newState() {
            arcs_.push();
            accept_costs.push(1e38);
            return arcs_.length() - 1;
        }
        virtual void addTransition(int from,int to,int output,float cost,int input) {
            Arc a;
            a.to = to;
            a.output = output;
            a.cost = cost;
            a.input = input;
            a.counter = 0;
            arcs_[from].push(a);
        }
        virtual void rescore(int from,int to,int output,float cost,int input) {
            narray<Arc> &arcs = arcs_[from];
            for(int i = 0; i < arcs.length(); i++) {
                if(arcs[i].input == input &&
                   arcs[i].to == to &&
                   (arcs[i].output == output)) {
                    arcs[i].cost = cost;
                    break;
                }
            }
        }
        virtual void setStart(int node) {
            start = node;
        }
        virtual void setAccept(int node,float cost=0.0) {
            accept_costs[node] = cost;
        }
        virtual int special(const char *s) {
            return 0;
        }
        virtual void bestpath(colib::nustring &result) {
            a_star(result, *this);
        }
        virtual void save(const char *path) {
            fst_write(path, *this);
        }
        virtual void load(const char *path) {
            fst_read(*this, path);
        }

        void expectation(IGenericFst &left, IGenericFst &right) {
            intarray inputs;
            intarray v1;
            intarray v2;
            intarray v3;
            intarray outputs;
            floatarray costs;

            if(!a_star_in_composition(inputs, v1, v2, v3, outputs, costs,
                                      left, *this, right)) {
                return;
            }

            // Now we need to infer what arcs have we passed through.
            // That won't be easy - in fact, it's ridiculously complicated
            // since we don't have any IDs on arcs.

            int n = v1.length();
            ASSERT(n == v2.length());
            ASSERT(n == v3.length());
            for(int i = 0; i < n - 1; i++) {
                // let's reconstruct the arc from v2[i] to v2[i + 1]

                // construct hashes
                archash lefthash, righthash;
                fill_archash(lefthash, left, v1[i], v1[i + 1], inputs[i],
                             /* is_input: */ false);
                fill_archash(righthash, right, v3[i], v3[i + 1], outputs[i],
                             /* is_input: */ true);

                // find the best arc

                int best_arc = -1; // -1 will mean no movement
                float best_arc_cost = 1e38;

                // There might be also a case when we've passed no arcs here.
                // That happens in input:epsilon, nothing, epsilon:output case.
                float leftcost = lefthash(0).value;
                float rightcost = righthash(0).value;
                if(v2[i] == v2[i + 1] && leftcost >= 0 && rightcost >= 0)
                    best_arc_cost = leftcost + rightcost;

                narray<Arc> &a = arcs_[v2[i]];
                for(int j = 0; j < a.length(); j++) {
                    if(a[j].to != v2[i + 1])
                        continue;
                    leftcost = lefthash(a[j].input).value;
                    rightcost = righthash(a[j].output).value;
                    if(leftcost < 0 || rightcost < 0)
                        continue;

                    float this_arc_cost = leftcost + a[j].cost + rightcost;

                    if(this_arc_cost < best_arc_cost) {
                        best_arc = j;
                        best_arc_cost = this_arc_cost;
                    }
                }

                // Update the counter of the best arc,
                // of course only we've passed any arc at all.
                if(best_arc >= 0)
                    a[best_arc].counter++;
            }
        }

        void maximization() {
            for(int i = 0; i < arcs_.length(); i++) {
                // rescore the arcs of the i-th vertex
                narray<Arc> &a = arcs_[i];

                // smooth
                for(int j = 0; j < a.length(); j++)
                    a[j].counter++;

                double total = 0;
                for(int j = 0; j < a.length(); j++)
                    total += a[j].counter;

                for(int j = 0; j < a.length(); j++) {
                    a[j].cost = a[j].counter / total;
                    a[j].counter = 0; // for the next E-step
                }
            }
        }
    };

}

namespace ocropus {
    ITrainableFst *make_StandardTrainableFst() {
        return new StandardTrainableFst();
    }
}

#endif
