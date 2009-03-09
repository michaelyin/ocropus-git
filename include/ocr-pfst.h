// Copyright 2007-2008 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz
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
// Project: ocr-pfst
// File: ocr-pfst.h
// Purpose: PFST public API
// Responsible: mezhirov
// Reviewer:
// Primary Repository:
// Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org

#ifndef h_ocr_pfst_
#define h_ocr_pfst_

/// \file ocr-pfst.h

#include "colib/colib.h"
#include "ocrinterfaces.h"

namespace ocropus {
    using namespace colib;

    /// \brief Make a readable/writable FST (independent on OpenFST).
    ///
    /// The FST will use OpenFST-compatible load() and save() routines,
    /// and A* for bestpath().
    IGenericFst *make_StandardFst();

    /// \brief Copy one FST to another.
    ///
    /// \param[out]     dst     The destination. Will be cleared before copying.
    /// \param[in]      src     The FST to copy.
    void fst_copy(IGenericFst &dst, IGenericFst &src);

    /// \brief Copy one FST to another, preserving only lowest-cost arcs.
    /// This is useful for visualization.
    ///
    /// \param[out]     dst     The destination. Will be cleared before copying.
    /// \param[in]      src     The FST to copy.
    void fst_copy_best_arcs_only(IGenericFst &dst, IGenericFst &src);

    /// \brief Insert one FST into another.
    ///
    /// This operation copies all the vertices of source FST into
    /// the destination, except for the start vertex, which is merged with
    /// the given one.
    ///
    /// \param[in,out]  dst     the FST to insert into
    /// \param[in]      src     the FST that will be inserted
    /// \param[in]      start   the destination FST's vertex
    ///                         that will be merged with the source FST's start
    /// \param[in]      accept  if unspecified,
    ///                         the source FST will just keep its accept costs,
    ///                         otherwise the new vertices will connect to
    ///                         the given vertex instead with their former
    ///                         accept costs.
    void fst_insert(IGenericFst &dst,
                    IGenericFst &src,
                    int start, int accept = -1);

    // ____________________________   A*   __________________________________

    /// \brief Search for the best path through the FST using A* algorithm.
    ///
    /// Technically, the best path always exists,
    /// but its cost might be over 1e37.
    /// So if your graph is disconnected,
    /// the beam search will go somewhere
    /// and take the extreme accept cost from there.
    ///
    /// All 4 output arrays will have the same length.
    ///
    /// \param[out] inputs      transition inputs along the path,
    ///                         padded by one 0 at the end
    ///                         (for the final jump out of the accept vertex)
    /// \param[out] vertices    vertices that the path goes through,
    ///                         including the starting vertex
    /// \param[out] outputs     transition outputs along the path,
    ///                         padded by one 0 at the end
    ///                         (for the final jump out of the accept vertex).
    ///                         Epsilons are not removed from the output.
    /// \param[out] costs       costs along the path; the last value is
    ///                         the accept cost of the final vertex
    /// \param[in]  fst         the FST to search
    bool a_star(intarray &inputs,
                intarray &vertices,
                intarray &outputs,
                floatarray &costs,
                IGenericFst &fst);

    // TODO: return cost
    /// \brief Simplified interface for a_star().
    ///
    /// \param[out] result      FST output with epsilons removed,
    ///                         converted to a nustring.
    /// \param[in]  fst         the FST to search
    void a_star(nustring &result, IGenericFst &fst);

    // TODO: document return value
    /// \brief Search for the best path through the composition of 2 FSTs
    ///        using A* algorithm.
    ///
    /// The interface is analogous to a_star(),
    /// but returns 2 arrays of vertices.
    ///
    /// This function uses A* on reversed FSTs separately,
    /// then uses the cost sum as a heuristic.
    /// No attempt to cache the heuristic is made.
    bool a_star_in_composition(intarray &inputs,
                               intarray &vertices1,
                               intarray &vertices2,
                               intarray &outputs,
                               floatarray &costs,
                               IGenericFst &fst1,
                               IGenericFst &fst2);

    // TODO: document return value
    /// \brief Search for the best path through the composition of 3 FSTs
    ///        using A* algorithm.
    /// The interface is analogous to a_star(),
    /// but returns 3 arrays of vertices.
    ///
    /// This function uses A* on reversed FSTs separately,
    /// then uses the cost sum as a heuristic.
    /// No attempt to cache the heuristic is made.
    bool a_star_in_composition(intarray &inputs,
                               intarray &vertices1,
                               intarray &vertices2,
                               intarray &vertices3,
                               intarray &outputs,
                               floatarray &costs,
                               IGenericFst &fst1,
                               IGenericFst &fst2,
                               IGenericFst &fst3);

};

#endif
