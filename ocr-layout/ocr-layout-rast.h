// -*- C++ -*-

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
// File: ocr-layout-rast.h
// Purpose: Extract textlines from a document image using RAST
//          For more information, please refer to the paper:
//          T. M. Breuel. "High Performance Document Layout Analysis",
//          Symposium on Document Image Understanding Technology, Maryland.
//          http://pubs.iupr.org/DATA/2003-breuel-sdiut.pdf
// Responsible: Faisal Shafait (faisal.shafait@dfki.de)
// Reviewer: 
// Primary Repository: 
// Web Sites: www.iupr.org, www.dfki.de

#ifndef h_ocrlayoutrast__
#define h_ocrlayoutrast__

#include "ocropus.h"
#include "ocr-layout-internal.h"

namespace ocropus {

    struct SegmentPageByRAST : colib::ISegmentPage {
        SegmentPageByRAST();
        ~SegmentPageByRAST() {}

        int  max_results;

        const char *description() {
            return "Segment page by RAST";
        }

        void init(const char **argv) {
            // nothing to be done
        }

        void set(const char* var,double value){
            if (strcmp(var,"max_results")==0)
                max_results = int(value);
        }   

        void segment(colib::intarray &image,colib::bytearray &in_not_inverted);
        void segment(colib::intarray &image,colib::bytearray &in_not_inverted,
                     colib::rectarray &extra_obstacles);
        void visualize(colib::intarray &result, colib::bytearray &in_not_inverted);
        
    private:
        void segmentInternal(colib::intarray &visualization, 
                             colib::intarray &image,
                             colib::bytearray &in_not_inverted, 
                             bool need_visualization,
                             rectarray &extra_obstacles);


    };

}

#endif
