// Copyright 2006-2008 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz
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
// Project: OCRopus
// File: voronoi-ocropus.h
// Purpose: Wrapper class for voronoi code
//
// Responsible: Faisal Shafait (faisal.shafait@dfki.de)
// Reviewer:
// Primary Repository:
// Web Sites: www.iupr.org, www.dfki.de


#ifndef h_voronoi_ocropus__
#define h_voronoi_ocropus__

#include "colib/colib.h"
#include "ocropus.h"

namespace ocropus {

    struct SegmentPageByVORONOI : ISegmentPage {
        p_int remove_noise; // remove noise from output image
        p_int nm;
        p_int sr;
        p_float fr;
        p_int ta;

        SegmentPageByVORONOI() {
            remove_noise.bind(this,"remove_noise",0,"remove noise parameter");
            nm.bind(this,"nm",-1,"nm parameter");
            sr.bind(this,"sr",-1,"nm parameter");
            fr.bind(this,"fr",-1,"nm parameter");
            ta.bind(this,"ta",-1,"nm parameter");
        }
        ~SegmentPageByVORONOI() {}

        const char *name() {
            return "segvoronoi";
        }

        const char *description() {
            return "segment page by Voronoi algorithm\n";
        }

        void segment(intarray &image,bytearray &in);
    };

    ISegmentPage *make_SegmentPageByVORONOI();

}


#endif
