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
// File: a-star.h
// Purpose: A* search
// Responsible: mezhirov
// Reviewer: 
// Primary Repository: 
// Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org

#ifndef h_a_star_
#define h_a_star_

/// \file a_star.h


#include "ocropus.h"
#include "langmods.h"

/// \namespace ocropus
namespace ocropus {
    using namespace colib;

    /// \brief Search for the best path through the FST.
    ///
    /// `inputs', `vertices', `outputs' and `costs' are output arrays.
    /// The format is analogous to beam_search().
    bool a_star(intarray &inputs,
                intarray &vertices,
                intarray &outputs,
                floatarray &costs,
                IGenericFst &fst);

    void a_star_backwards(floatarray &costs_for_all_nodes,
                          IGenericFst &fst);

    bool a_star_in_composition(intarray &inputs,
                               intarray &vertices1,
                               intarray &vertices2,
                               intarray &outputs,
                               floatarray &costs,
                               IGenericFst &fst1,
                               IGenericFst &fst2);
    
    bool a_star_in_composition(intarray &inputs,
                               intarray &vertices1,
                               intarray &vertices2,
                               intarray &vertices3,
                               intarray &outputs,
                               floatarray &costs,
                               IGenericFst &fst1,
                               IGenericFst &fst2,
                               IGenericFst &fst3);

    void a_star(nustring &result, IGenericFst &fst);
}

#endif
