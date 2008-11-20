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
// File: a-star.cc
// Purpose: A* search
// Responsible: mezhirov
// Reviewer:
// Primary Repository:
// Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org

#ifndef a_star_h_
#define a_star_h_

#include "ocropus.h"
#include "langmods.h"

using namespace colib;
using namespace ocropus;

namespace {
    // OK, this turned out to be scarier than I thought.
    // We need heap reimplementation here since we need to change costs on the fly.
    // We maintain `heapback' array for that.
    class Heap {
        intarray heap;       // the priority queue
        intarray heapback;   // heap[heapback[node]] == node; -1 if not in the heap
        floatarray costs; // the cost of the node on the heap

        int left(int i) {  return 2 * i + 1; }
        int right(int i) {  return 2 * i + 2; }
        int parent(int i) {  return (i - 1) / 2; }

        int rotate(int i) {
            int size = heap.length();
            int j = left(i);
            int k = right(i);
            if(k < size && costs[k] < costs[j]) {
                if(costs[k] < costs[i]) {
                    heapswap(k, i);
                    return k;
                }
            } else if (j < size) {
                if(costs[j] < costs[i]) {
                    heapswap(j, i);
                    return j;
                }
            }
            return i;
        }

        // Swap 2 nodes on the heap, maintaining heapback.
        void heapswap(int i, int j) {
            int t = heap[i];
            heap[i] = heap[j];
            heap[j] = t;

            float c = costs[i];
            costs[i] = costs[j];
            costs[j] = c;

            heapback[heap[i]] = i;
            heapback[heap[j]] = j;
        }

        // Fix the heap invariant broken by increasing the cost of heap node i.
        void heapify_down(int i) {
            while(1) {
                int k = rotate(i);
                if(i == k)
                    return;
                i = k;
            }
        }

        // Fix the heap invariant broken by decreasing the cost of heap node i.
        void heapify_up(int i) {
            while(i) {
                int j = parent(i);
                if(costs(i) < costs(j)) {
                    heapswap(i, j);
                    i = j;
                } else {
                    return;
                }
            }
        }

    public:
        /// Create a heap storing node indices from 0 to n - 1.
        Heap(int n) : heapback(n) {
            fill(heapback, -1);
        }

        int length() {
            return heap.length();
        }

        /// Return the item with the least cost and remove it from the heap.
        int pop() {
            heapswap(0, heap.length() - 1);
            int result = heap.pop();
            costs.pop();
            heapify_down(0);
            heapback[result] = -1;
            return result;
        }

        /// Push the node in the heap if it's not already there, otherwise promote.
        /// @returns True if the heap was changed, false if the item was already
        ///          in the heap and with a better cost.
        bool push(int node, float cost) {
            int i = heapback[node];
            if(i != -1) {
                if(cost < costs[i]) {
                    costs[i] = cost;
                    heapify_up(i);
                    return true;
                }
                return false;
            } else {
                heap.push(node);
                costs.push(cost);
                heapback[node] = heap.length() - 1;
                heapify_up(heap.length() - 1);
                return true;
            }
        }

    };

    class AStarSearch {
        IGenericFst &fst;

        intarray came_from; // the previous node in the best path; -1 for unseen,
                            // self for the start
        int accepted_from;
        float g_accept;     // best cost for accept so far
        int n;              // the number of nodes; also the virtual accept index
        Heap heap;

    public:
        floatarray g;       // the cost of the best path from the start to here
    public:

        virtual double heuristic(int index) {
            return 0;
        }

        AStarSearch(IGenericFst &fst): fst(fst),
                                            accepted_from(-1),
                                            heap(fst.nStates() + 1) {
            n = fst.nStates();
            came_from.resize(n);
            fill(came_from, -1);
            g.resize(n);

            // insert the start node
            int s = fst.getStart();
            g[s] = 0;
            came_from(s) = s;
            heap.push(s, heuristic(s));
        }
        virtual ~AStarSearch() {}

        bool step() {
            int node = heap.pop();
            if(node == n)
                return true;  // accept has popped up

            // get outbound arcs
            intarray inputs;
            intarray targets;
            intarray outputs;
            floatarray costs;
            fst.arcs(inputs, targets, outputs, costs, node);
            for(int i = 0; i < targets.length(); i++) {
                int t = targets[i];
                if(came_from[t] == -1 || g[node] + costs[i] < g[t]) {
                    // relax the edge
                    came_from[t] = node;
                    g[t] = g[node] + costs[i];
                    heap.push(t, g[t] + heuristic(t));
                }
            }
            if(accepted_from == -1
            || g[node] + fst.getAcceptCost(node) < g_accept) {
                // relax the accept edge
                accepted_from = node;
                g_accept = g[node] + fst.getAcceptCost(node);
                heap.push(n, g_accept);
            }
            return false;
        }

        bool loop() {
            while(heap.length()) {
                if(step())
                    return true;
            }
            return false;
        }

        bool reconstruct_vertices(intarray &result_vertices) {
            intarray vertices;
            if(accepted_from == -1)
                return false;
            vertices.push(accepted_from);
            int last = accepted_from;
            int next;
            while((next = came_from[last]) != last) {
                vertices.push(next);
                last = next;
            }
            reverse(result_vertices, vertices);
            return true;
        }

        bool reconstruct_edges(intarray &inputs,
                               intarray &outputs,
                               floatarray &costs,
                               intarray &vertices) {
            int n = vertices.length();
            inputs.resize(n);
            outputs.resize(n);
            costs.resize(n);
            for(int i = 0; i < n - 1; i++) {
                int source = vertices[i];
                int target = vertices[i + 1];
                intarray out_ins;
                intarray out_targets;
                intarray out_outs;
                floatarray out_costs;
                fst.arcs(out_ins, out_targets, out_outs, out_costs, source);

                costs[i] = g[target] - g[source] + 1e-5; // to ensure catching it

                // find the best arc
                for(int j = 0; j < out_targets.length(); j++) {
                    if(out_targets[j] != target) continue;
                    if(out_costs[j] < costs[i]) {
                        inputs[i] = out_ins[j];
                        outputs[i] = out_outs[j];
                        costs[i] = out_costs[j];
                    }
                }
            }
            inputs[n - 1] = 0;
            outputs[n - 1] = 0;
            costs[n - 1] = fst.getAcceptCost(vertices[n - 1]);
            // FIXME
            throw "missing return statement, don't know what to return here --tmb";
        }
    };


    struct AStarCompositionSearch : AStarSearch {
        floatarray &g1, &g2; // well, that's against our convention,
        CompositionFst &c;   // but I let it go since it's so local.

        virtual double heuristic(int index) {
            int i1, i2;
            c.splitIndex(i1, i2, index);
            return g1[i1] + g2[i2];
        }

        AStarCompositionSearch(floatarray &g1, floatarray &g2, CompositionFst &c) : AStarSearch(c), g1(g1), g2(g2), c(c) {
        }
    };

    struct AStarN : AStarSearch {
        narray<floatarray> &g;
        narray< autodel<CompositionFst> > &c;
        int n;

        virtual double heuristic(int index) {
            double result = 0;
            for(int i = 0; i < n - 1; i++) {
                int i1, i2;
                c[i]->splitIndex(i1, i2, index);
                result += g[i][i1];
                index = i2;
            }
            result += g[n - 1][index];
            return result;
        }

        AStarN(narray<floatarray> &g, narray< autodel<CompositionFst> > &c)
        : AStarSearch(*c[0]), g(g), c(c), n(g.length()) {
            CHECK_ARG(c.length() == n - 1);
        }
    };

    bool a_star2_internal(colib::intarray &inputs,
                                    colib::intarray &vertices1,
                                    colib::intarray &vertices2,
                                    colib::intarray &outputs,
                                    colib::floatarray &costs,
                                    colib::IGenericFst &fst1,
                                    colib::IGenericFst &fst2,
                                    CompositionFst& composition) {
        intarray vertices;
        floatarray g1, g2;
        a_star_backwards(g1, fst1);
        a_star_backwards(g2, fst2);
        AStarCompositionSearch a(g1, g2, composition);
        if(!a.loop())
            return false;
        if(!a.reconstruct_vertices(vertices))
            return false;
        if(!a.reconstruct_edges(inputs, outputs, costs, vertices))
            return false;
        composition.splitIndices(vertices1, vertices2, vertices);
        return true;
    }

    // Pointers - eek... I know.
    // But we don't have a non-owning wrapper yet, do we?
    // I don't like the idea of autodels around all IGenericFsts
    // just to take them back before the destruction. --IM
    bool a_star_N_internal(colib::intarray &inputs,
                           narray<intarray> &vertices,
                           colib::intarray &outputs,
                           colib::floatarray &costs,
                           narray<IGenericFst *> &fsts,
                           narray< autodel<CompositionFst> > &compositions) {
        intarray v;
        int n = fsts.length();
        narray<floatarray> g(n);
        for(int i = 0; i < n; i++)
            a_star_backwards(g[i], *fsts[i]);
        AStarN a(g, compositions);
        if(!a.loop())
            return false;
        if(!a.reconstruct_vertices(v))
            return false;
        if(!a.reconstruct_edges(inputs, outputs, costs, v))
            return false;
        intarray t;
        for(int i = 0; i < n - 1; i++) {
            compositions[i]->splitIndices(vertices[i], t, v);
            move(v, t);
        }
        move(vertices[n - 1], v);
        return true;
    }

    bool a_star_N(colib::intarray &inputs,
                               narray<intarray> &vertices,
                               colib::intarray &outputs,
                               colib::floatarray &costs,
                               narray<IGenericFst *> &fsts) {
        int n = fsts.length();
        vertices.resize(n);
        if(n == 0)
            return false;
        if(n == 1)
            return a_star(inputs, vertices[0], outputs, costs, *fsts[0]);

        narray< autodel<CompositionFst> > compositions(n - 1);
        compositions[n - 2] = make_CompositionFst(fsts[n-2], fsts[n-1]);
        for(int i = n - 3; i >= 0; i--)
           compositions[i] = make_CompositionFst(fsts[i], &*compositions[i+1]);

        bool result;
        try {
            result = a_star_N_internal(inputs, vertices, outputs, costs, fsts,
                                       compositions);
        } catch(...) {
            for(int i = 0; i < n - 1; i++) {
                compositions[i]->move1();
                compositions[i]->move2();
            }
            throw;
        }
        for(int i = 0; i < n - 1; i++) {
            compositions[i]->move1();
            compositions[i]->move2();
        }
        return result;
    }

}

namespace ocropus {
    bool a_star(intarray &inputs,
                intarray &vertices,
                intarray &outputs,
                floatarray &costs,
                IGenericFst &fst) {
        AStarSearch a(fst);
        if(!a.loop())
            return false;
        if(!a.reconstruct_vertices(vertices))
            return false;
        if(!a.reconstruct_edges(inputs, outputs, costs, vertices))
            return false;
        return true;
    }

    void a_star_backwards(floatarray &costs_for_all_nodes, IGenericFst &fst) {
        autodel<IGenericFst> reverse(make_StandardFst());
        fst_copy_reverse(*reverse, fst, true); // creates an extra vertex
        AStarSearch a(*reverse);
        a.loop();
        copy(costs_for_all_nodes, a.g);
        costs_for_all_nodes.pop(); // remove the extra vertex
    }

    bool a_star_in_composition(colib::intarray &inputs,
                               colib::intarray &vertices1,
                               colib::intarray &vertices2,
                               colib::intarray &outputs,
                               colib::floatarray &costs,
                               colib::IGenericFst &fst1,
                               colib::IGenericFst &fst2) {
        autodel<CompositionFst> composition(make_CompositionFst(&fst1, &fst2));
        bool result;
        try {
            result = a_star2_internal(inputs, vertices1, vertices2, outputs,
                                      costs, fst1, fst2, *composition);
        } catch(...) {
            composition->move1();
            composition->move2();
            throw;
        }
        composition->move1();
        composition->move2();
        return result;
    }

    bool a_star_in_composition(colib::intarray &inputs,
                 colib::intarray &vertices1,
                 colib::intarray &vertices2,
                 colib::intarray &vertices3,
                 colib::intarray &outputs,
                 colib::floatarray &costs,
                 colib::IGenericFst &fst1,
                 colib::IGenericFst &fst2,
                 colib::IGenericFst &fst3) {
        narray<intarray> v;
        narray<IGenericFst *> fsts(3);
        fsts[0] = &fst1;
        fsts[1] = &fst2;
        fsts[2] = &fst3;

        bool result = a_star_N(inputs, v, outputs, costs, fsts);
        move(vertices1, v[0]);
        move(vertices2, v[1]);
        move(vertices3, v[2]);
        return result;
    }

    void a_star(nustring &result, IGenericFst &fst) {
        intarray inputs;
        intarray vertices;
        intarray outputs;
        floatarray costs;
        a_star(inputs, vertices, outputs, costs, fst);
        for(int i = 0; i < outputs.length(); i++) {
            if(outputs[i])
                result.push(nuchar(outputs[i]));
        }
    }

};

#endif
