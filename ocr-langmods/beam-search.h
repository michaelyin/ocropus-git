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
// File: beam-search.h
// Purpose:
// Responsible: mezhirov
// Reviewer:
// Primary Repository:
// Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org

#ifndef h_beam_search_
#define h_beam_search_

/// \file beam-search.h


#include "ocropus.h"
#include "langmods.h"

/// \namespace ocropus
namespace ocropus {

    using namespace colib;

    enum {DEFAULT_BEAM_WIDTH = 100};

    /// \brief Search for the best path through the FST.
    ///
    /// All 4 output arrays will have the same length.
    /// The result is not exact but in practice is likely to be.
    ///
    /// Epsilons are not removed from the output.
    ///
    /// Technically, the best path always exists,
    /// but its cost might be over 1e37.
    /// So if your graph is disconnected,
    /// the beam search will go somewhere
    /// and take the extreme accept cost from there.
    ///
    /// \param[out] inputs      transition inputs along the path,
    ///                         padded by one 0 at the end
    ///                         (for the final jump out of the accept vertex)
    /// \param[out] vertices    vertices that the path goes through,
    ///                         including the starting vertex
    /// \param[out] outputs     transition outputs along the path,
    ///                         padded by one 0 at the end
    ///                         (for the final jump out of the accept vertex)
    /// \param[out] costs       costs along the path; the last value is
    ///                         the accept cost of the final vertex
    /// \param[in]  fst         the FST to search
    /// \param[in]  beam_width  optional parameter; the more the beam width,
    ///                         the less the incorrect result probability,
    ///                         but the slower it works
    /// \param[in]  override_start      optional parameter to search
    ///                                 starting from an arbitrary vertex
    /// \param[in]  override_finish     optional parameter to search
    ///                                 considering only the given vertex
    ///                                 to be final (with the accept cost of 0).
    void beam_search(intarray &inputs,
                     intarray &vertices,
                     intarray &outputs,
                     floatarray &costs,
                     IGenericFst &fst,
                     int beam_width = DEFAULT_BEAM_WIDTH,
                     int override_start = -1,
                     int override_finish = -1);


    /// The simplified interface for beam_search().
    void beam_search(nustring &output,
                     IGenericFst &fst,
                     int beam_width = DEFAULT_BEAM_WIDTH);


    /// Search for the best path through the composition of two FSTs.
    //
    /// The semantics of output arrays is the same as for beam_search().
    void beam_search_in_composition(intarray &inputs,
                                    intarray &vertices1,
                                    intarray &vertices2,
                                    intarray &outputs,
                                    floatarray &costs,
                                    IGenericFst &fst1,
                                    IGenericFst &fst2,
                                    int beam_width = DEFAULT_BEAM_WIDTH,
                                    int override_start = -1,
                                    int override_finish = -1);

    void beam_search_in_composition(intarray &outputs,
                                    floatarray &costs,
                                    IGenericFst &fst1,
                                    IGenericFst &fst2,
                                    int beam_width = DEFAULT_BEAM_WIDTH,
                                    int override_start = -1,
                                    int override_finish = -1);

    void beam_search_in_composition(intarray &outputs,
                                    IGenericFst &fst1,
                                    IGenericFst &fst2,
                                    int beam_width = DEFAULT_BEAM_WIDTH,
                                    int override_start = -1,
                                    int override_finish = -1);

    void beam_search_in_composition(nustring &outputs,
                                    IGenericFst &fst1,
                                    IGenericFst &fst2,
                                    int beam_width = DEFAULT_BEAM_WIDTH,
                                    int override_start = -1,
                                    int override_finish = -1);
}

#endif
