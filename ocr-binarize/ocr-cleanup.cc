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
// File: ocr-binarize-sauvola.cc
// Responsible: Faisal Shafait (faisal.shafait@dfki.de)
// Reviewer:
// Primary Repository:
// Web Sites: www.iupr.org, www.dfki.de

#include "ocropus.h"
#include "colib/iarith.h"

#define MAXVAL 256

using namespace iulib;
using namespace colib;

namespace ocropus {
    static void remove_noise_boxes(bytearray &image,int mw,int mh){
        intarray segmentation;
        segmentation = image;
        using namespace narray_ops;
        sub(255,segmentation);
        label_components(segmentation);
        narray<rectangle> bboxes;
        bounding_boxes(bboxes,segmentation);
        debugf("info","got %d bboxes\n",bboxes.length());
        for(int i=1;i<bboxes.length();i++) {
            rectangle b = bboxes(i);
            if(b.width()<=mw && b.height()<=mh) {
                for(int x=b.x0;x<b.x1;x++)
                    for(int y=b.y0;y<b.y1;y++)
                        if(segmentation(x,y)==i)
                            image(x,y) = 255;
            }
        }
    }

    static void count_noise_boxes(intarray &counts,bytearray &image,int mw,int mh){
        intarray segmentation;
        segmentation = image;
        using namespace narray_ops;
        sub(255,segmentation);
        label_components(segmentation);
        narray<rectangle> bboxes;
        bounding_boxes(bboxes,segmentation);
        counts.resize(2);
        counts = 0;
        for(int i=1;i<bboxes.length();i++) {
            rectangle b = bboxes(i);
            if(b.width()<=mw && b.height()<=mh) 
                counts(0)++;
            else
                counts(1)++;
        }
    }

    struct RmHalftone : ICleanupBinary {
        p_float factor;
        p_int threshold;
        RmHalftone() {
            factor.bind(this,"factor",3.0,"trigger removal if # small components > factor * # large components");
            threshold.bind(this,"threshold",4,"small components fit into this size box");
        }
        const char *description() {
            return "remove halftoning (defaults for 300dpi images)";
        }

        const char *name() {
            return "rmhalftone";
        }

        void cleanup(bytearray &out,bytearray &in_) {
            bytearray in;
            in = in_;
            // make sure it's binary
            for(int i=0;i<out.length();i++)
                if(in[i]>128) in[i] = 255;
            out = in;
            intarray counts;
            count_noise_boxes(counts,in,threshold,threshold);
            if(counts(0)>factor*counts(1)) {
                debugf("info","removing halftoning\n");
                // get rid of halftoning
                binary_close_rect(out,3,1);
                binary_close_rect(out,1,3);
                binary_open_circle(out,1);
                for(int i=0;i<out.length();i++)
                    if(in[i]) out[i] = 255;
            }
        }
    };

    struct RmUnderline : ICleanupBinary {
        const char *description() {
            return "remove underlines (defaults for 300dpi images)";
        }

        const char *name() {
            return "rmunderline300";
        }

        void cleanup(bytearray &out,bytearray &in_) {
            bytearray in;
            in = in_;
            // make sure it's binary
            for(int i=0;i<out.length();i++)
                if(in[i]>128) in[i] = 255;
            out = in;
            // get rid of underlines
            bytearray underlines;
            underlines = in;
            binary_erode_rect(underlines,2,1);
            binary_close_rect(underlines,200,1);
            binary_erode_rect(underlines,1,5);
            for(int i=0;i<out.length();i++)
                if(underlines[i]==0) out[i] = 255;
        }
    };

    struct RmBig: ICleanupBinary {
        RmBig() {
            pdef("mw",300,"maximum width");
            pdef("mh",100,"maximum height");
            pdef("minaspect",0.03,"minimum aspect ratio");
            pdef("maxaspect",30.0,"maximum aspect ratio");
        }
        const char *description() {
            return "remove big components (defaults for 300dpi images)";
        }

        const char *name() {
            return "rmbig";
        }

        void cleanup(bytearray &image,bytearray &in) {
            image = in;
            // make sure it's binary
            for(int i=0;i<image.length();i++)
                if(image[i]>128) image[i] = 255;
            intarray segmentation;
            segmentation = image;

            // compute bounding boxes
            using namespace narray_ops;
            sub(255,segmentation);
            label_components(segmentation);
            narray<rectangle> bboxes;
            bounding_boxes(bboxes,segmentation);
            debugf("info","got %d bboxes\n",bboxes.length());

            // remove large components
            int mw = pgetf("mw");
            int mh = pgetf("mh");
            float minaspect = pgetf("minaspect");
            float maxaspect = pgetf("maxaspect");
            for(int i=1;i<bboxes.length();i++) {
                rectangle b = bboxes(i);
                float aspect = b.height() * 1.0/b.width();
                if(b.width()>=mw || b.height()>=mh || aspect<minaspect || aspect>maxaspect) {
                    for(int x=b.x0;x<b.x1;x++)
                        for(int y=b.y0;y<b.y1;y++)
                            if(segmentation(x,y)==i)
                                image(x,y) = 255;
                }
            }
        }
    };

    struct AutoInvert : ICleanupBinary {
        AutoInvert() {
            pdef("fraction",0.7,"fraction above which to invert");
            pdef("minheight",100,"minimum height for autoinvert");
        }

        const char *description() {
            return "automatically invert white-on-black text";
        }

        const char *name() {
            return "autoinvert";
        }

        void cleanup(bytearray &out,bytearray &in) {
            out = in;
            if(out.dim(1)<pgetf("minheight")) return;
            // make sure it's binary
            for(int i=0;i<out.length();i++)
                if(out[i]>128) out[i] = 255;
            out = in;
            // get rid of underlines
            int count = 0;
            for(int i=0;i<out.length();i++)
                if(out[i]==0) count++;
            if(count>=pgetf("fraction")*out.length())
                for(int i=0;i<out.length();i++)
                    out[i] = 255*!out[i];
        }
    };

    ICleanupBinary *make_RmBig() {
        return new RmBig();
    }

    ICleanupBinary *make_RmHalftone() {
        return new RmHalftone();
    }

    ICleanupBinary *make_RmUnderline() {
        return new RmUnderline();
    }

    ICleanupBinary *make_AutoInvert() {
        return new AutoInvert();
    }
}
