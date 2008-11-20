// Copyright 2007 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz
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
// File: beam-search.cc
// Purpose:
// Responsible: mezhirov
// Reviewer:
// Primary Repository:
// Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org

#ifndef beam_search_h_
#define beam_search_h_

#include "ocropus.h"
#include "langmods.h"

using namespace colib;
using namespace ocropus;

namespace {
    // FIXME: this version of NBest (with add_replacing_id) proposed for iulib
    class NBest {
    public:
        int n;
        int fill;
        intarray ids;
        intarray tags;
        doublearray values;
        /// constructor for a NBest data structure of size n
        NBest(int n):n(n) {
            ids.resize(n+1);
            tags.resize(n+1);
            values.resize(n+1);
            clear();
        }
        /// remove all elements
        void clear() {
            fill = 0;
            for(int i=0;i<=n;i++) ids[i] = -1;
            for(int i=0;i<=n;i++) values[i] = -1e38;
        }
        void move_value(int id, int tag, double value, int start, int end) {
            int i = start;
            while(i>end) {
                if(values[i-1]>=value) break;
                values[i] = values[i-1];
                tags[i] = tags[i-1];
                ids[i] = ids[i-1];
                i--;
            }
            values[i] = value;
            tags[i] = tag;
            ids[i] = id;
        }


        /// add the id with the corresponding value
        bool add(int id, int tag, double value) {
            if(fill==n) {
                if(values[n-1]>=value) return false;
                move_value(id, tag, value, n-1, 0);
            } else if(fill==0) {
                values[0] = value;
                ids[0] = id;
                tags[0] = tag;
                fill++;
            } else {
                move_value(id, tag, value, fill, 0);
                fill++;
            }
            return true;
        }

        int find_id(int id) {
            for(int i = 0; i < fill; i++) {
                if(ids[i] == id)
                    return i;
            }
            return -1;
        }

        /// This function will move the existing id up
        /// instead of creating a new one.
        bool add_replacing_id(int id, int tag, double value) {
            int former = find_id(id);
            if(former == -1)
                return add(id, tag, value);
            if(values[former]>=value)
                return false;
            move_value(id, tag, value, former, 0);
            return true;
        }

        /// get the value corresponding to rank i
        double value(int i) {
            if(unsigned(i)>=unsigned(n)) throw "range error";
            return values[i];
        }
        int tag(int i) {
            if(unsigned(i)>=unsigned(n)) throw "range error";
            return tags[i];
        }
        /// get the id corresponding to rank i
        int operator[](int i) {
            if(unsigned(i)>=unsigned(n)) throw "range error";
            return ids[i];
        }
        /// get the number of elements in the NBest structure (between 0 and n)
        int length() {
            return fill;
        }
    };


    struct Trail {
        intarray ids;
        intarray vertices;
        intarray outputs;
        floatarray costs;
        float total_cost;
        int vertex;
    };

    void copy(Trail &a, Trail &b) {
        copy(a.ids, b.ids);
        copy(a.vertices, b.vertices);
        copy(a.outputs, b.outputs);
        copy(a.costs, b.costs);
        a.total_cost = b.total_cost;
        a.vertex = b.vertex;
    }

    // Traverse the beam; accept costs are not traversed
    void radiate(narray<Trail> &new_beam,
                 narray<Trail> &beam,
                 IGenericFst &fst,
                 double bound,
                 int beam_width) {
        NBest nbest(beam_width);
        intarray all_ids;
        intarray all_targets;
        intarray all_outputs;
        floatarray all_costs;
        intarray parents;

        for(int i = 0; i < beam.length(); i++) {
            intarray ids;
            intarray targets;
            intarray outputs;
            floatarray costs;
            fst.arcs(ids, targets, outputs, costs, beam[i].vertex);
            float max_acceptable_cost = bound - beam[i].total_cost;
            if(max_acceptable_cost < 0) continue;
            for(int j = 0; j < targets.length(); j++) {
                if(costs[j] >= max_acceptable_cost)
                    continue;
                nbest.add_replacing_id(targets[j], all_targets.length(),
                                       -costs[j]-beam[i].total_cost);
                all_ids.push(ids[j]);
                all_targets.push(targets[j]);
                all_outputs.push(outputs[j]);
                all_costs.push(costs[j]);
                parents.push(i);
            }
        }

        // build new beam
        new_beam.resize(nbest.length());
        for(int i = 0; i < new_beam.length(); i++) {
            Trail &t = new_beam[i];
            int k = nbest.tag(i);
            Trail &parent = beam[parents[k]];
            copy(t.ids, parent.ids);
            t.ids.push(all_ids[k]);
            copy(t.vertices, parent.vertices);
            t.vertices.push(beam[parents[k]].vertex);
            copy(t.outputs, parent.outputs);
            t.outputs.push(all_outputs[k]);
            copy(t.costs, parent.costs);
            t.costs.push(all_costs[k]);
            t.total_cost = -nbest.value(i);
            t.vertex = all_targets[k];
        }
    }

    void try_accepts(Trail &best_so_far,
                     narray<Trail> &beam,
                     IGenericFst &fst) {
        float best_cost = best_so_far.total_cost;
        for(int i = 0; i < beam.length(); i++) {
            float accept_cost = fst.getAcceptCost(beam[i].vertex);
            float candidate = beam[i].total_cost + accept_cost;
            if(candidate < best_cost) {
                copy(best_so_far, beam[i]);
                best_cost = best_so_far.total_cost = candidate;
            }
        }
    }

    void try_finish(Trail &best_so_far,
                     narray<Trail> &beam,
                     IGenericFst &fst,
                     int finish) {
        float best_cost = best_so_far.total_cost;
        for(int i = 0; i < beam.length(); i++) {
            if(beam[i].vertex != finish) continue;
            float candidate = beam[i].total_cost;
            if(candidate < best_cost) {
                copy(best_so_far, beam[i]);
                best_cost = best_so_far.total_cost = candidate;
            }
        }
    }
}


namespace ocropus {

    void beam_search(colib::intarray &ids,
                     colib::intarray &vertices,
                     colib::intarray &outputs,
                     colib::floatarray &costs,
                     IGenericFst &fst,
                     int beam_width,
                     int override_start,
                     int override_finish) {
        narray<Trail> beam(1);
        Trail &start = beam[0];
        start.total_cost = 0;
        if(override_start != -1)
            start.vertex = override_start;
        else
            start.vertex = fst.getStart();

        Trail best_so_far;
        best_so_far.vertex = start.vertex;
        best_so_far.total_cost = fst.getAcceptCost(start.vertex);

        while(beam.length()) {
            narray<Trail> new_beam;
            if(override_finish != -1)
                try_finish(best_so_far, beam, fst, override_finish);
            else
                try_accepts(best_so_far, beam, fst);
            double bound = best_so_far.total_cost;
            radiate(new_beam, beam, fst, bound, beam_width);
            move(beam, new_beam);
        }

        move(ids, best_so_far.ids);
        ids.push(0);
        move(vertices, best_so_far.vertices);
        vertices.push(best_so_far.vertex);
        move(outputs, best_so_far.outputs);
        outputs.push(0);
        move(costs, best_so_far.costs);
        costs.push(fst.getAcceptCost(best_so_far.vertex));
    }

    void beam_search(nustring &result,
                     colib::IGenericFst &fst,
                     int beam_width) {
        intarray ids;
        intarray vertices;
        intarray outputs;
        floatarray costs;
        beam_search(ids, vertices, outputs, costs, fst);
        for(int i = 0; i < outputs.length(); i++) {
            if(outputs[i])
                result.push(nuchar(outputs[i]));
        }
    }

    // FIXME this needs to do the search without an explicit composition --tmb

    void beam_search_in_composition(colib::intarray &inputs,
                                    colib::intarray &vertices1,
                                    colib::intarray &vertices2,
                                    colib::intarray &outputs,
                                    colib::floatarray &costs,
                                    colib::IGenericFst &fst1,
                                    colib::IGenericFst &fst2,
                                    int beam_width,
                                    int override_start,
                                    int override_finish) {
        autodel<CompositionFst> composition(
            make_CompositionFst(&fst1, &fst2));
        try {
            intarray vertices;
            beam_search(inputs, vertices, outputs, costs, *composition,
                        beam_width, override_start, override_finish);
            composition->splitIndices(vertices1, vertices2, vertices);
        } catch(...) {
            composition->move1();
            composition->move2();
            throw;
        }
        composition->move1();
        composition->move2();
    }

    void beam_search_in_composition(colib::intarray &outputs,
                                    colib::floatarray &costs,
                                    colib::IGenericFst &fst1,
                                    colib::IGenericFst &fst2,
                                    int beam_width,
                                    int override_start,
                                    int override_finish) {
        intarray inputs, vertices1, vertices2;
        beam_search_in_composition(inputs,vertices1,vertices2,outputs,costs,fst1,fst2,
                                   beam_width,override_start,override_finish);
    }
    void beam_search_in_composition(colib::intarray &outputs,
                                    colib::IGenericFst &fst1,
                                    colib::IGenericFst &fst2,
                                    int beam_width,
                                    int override_start,
                                    int override_finish) {
        intarray inputs, vertices1, vertices2;
        floatarray costs;
        beam_search_in_composition(inputs,vertices1,vertices2,outputs,costs,fst1,fst2,
                                   beam_width,override_start,override_finish);
    }

    void beam_search_in_composition(colib::nustring &result,
                                    colib::IGenericFst &fst1,
                                    colib::IGenericFst &fst2,
                                    int beam_width,
                                    int override_start,
                                    int override_finish) {
        intarray inputs, vertices1, vertices2, outputs;
        floatarray costs;
        beam_search_in_composition(inputs,vertices1,vertices2,outputs,costs,fst1,fst2,
                                   beam_width,override_start,override_finish);
        result.of(outputs);
    }
}

#endif
