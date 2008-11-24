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
// File: fst-em.h
// Purpose: EM training of FSTs
// Responsible: mezhirov
// Reviewer: 
// Primary Repository: 
// Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org


#ifndef h_fst_em_
#define h_fst_em_

#include "ocropus.h"
#include "langmods.h"

namespace ocropus {
    using namespace colib;

    struct ITrainableFst : IGenericFst {
        /// Update internal statistics reflecting the apperance of this FST
        /// in composition with the others, in this order: left o this o right.
        virtual void expectation(IGenericFst &left,
                                 IGenericFst &right)=0;

        /// Use the statistics accumulated with calls to expectation()
        /// to update the weights, maximizing the likelihood of what was seen.
        /// Reset the statistics for the new sequence of expectation() calls.
        virtual void maximization()=0;
    };

    ITrainableFst *make_StandardTrainableFst();
}

#endif
