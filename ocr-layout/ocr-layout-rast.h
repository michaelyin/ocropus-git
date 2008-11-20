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
#include "ocr-layout.h"

namespace ocropus {
    using namespace colib;

    struct line{
        float c,m,d; // c is y-intercept, m is slope, d is the line of descenders
        float start,end,top,bottom; // start and end of line segment
        float istart,iend; //actual start and end of line segment in the image
        float xheight;

        line() {}
        line(TextLine &tl);
        TextLine getTextLine();
    };

    // FIXME
    // this class exposes implementation details that shouldn't be exposed
    // put the class definition inside the source file
    // --tmb

    struct SegmentPageByRAST : ISegmentPage {
        SegmentPageByRAST();
        ~SegmentPageByRAST() {}

        // Overlap threshold for grouping paragraphs into columns
        float column_threshold;

        int  id;
        int  max_results;
        narray<int> val;
        narray<int> ro_index;

        const char *description() {
            return "Segment page by RAST";
        }

        void init(const char **argv) {
            // nothing to be done
        }

        // FIXME method name --tmb
        void roSort(narray<TextLine> &textlines,
                    rectarray &columns,
                    CharStats &charstats);

        // FIXME method name --tmb
        void groupPara(rectarray &paragraphs,
                       narray<TextLine> &textlines,
                       CharStats &charstats);

        // FIXME method name --tmb
        void getCol(rectarray &columns,
                    rectarray &paragraphs);


        // FIXME method name --tmb
        void getCol(rectarray &textcolumns,
                    narray<TextLine> &textlines,
                    rectarray &gutters);

        void color(intarray &image, bytearray &in,
                   narray<TextLine> &textlines,
                   rectarray &columns);


        void segment(intarray &image,bytearray &in_not_inverted);
        void segment(intarray &image,bytearray &in_not_inverted,rectarray &extra_obstacles);
        void visualize(intarray &result, bytearray &in_not_inverted);

        void set(const char*,double);

    private:
        void visualizeLayout(intarray &result, bytearray &in_not_inverted, narray<TextLine> &textlines, rectarray &columns,  CharStats &charstats);
        void segmentInternal(intarray &visualization, intarray &image,bytearray &in_not_inverted, bool need_visualization,rectarray &extra_obstacles);
        void visit(int k, narray<bool> &lines_dag);
        void depthFirstSearch(narray<bool> &lines_dag);


    };

    ISegmentPage *make_SegmentPageByRAST();
    void visualize_segmentation_by_RAST(intarray &result, bytearray &in_not_inverted);
}

#endif
