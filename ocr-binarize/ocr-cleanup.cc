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
#if 0
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
#endif

    static int max_n = 10000;

    static void count_noise_boxes(intarray &counts,bytearray &image,int mw,int mh){
        intarray segmentation;
        segmentation = image;
        using namespace narray_ops;
        sub(255,segmentation);
        int n = label_components(segmentation);
        if(n>max_n) throw "too many connected components";
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
        p_int max_n_;
        RmHalftone() {
            max_n_.bind(this,"max_n",10000,"maximum number of connected components");
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
            max_n = max_n_;
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
            pdef("max_n",10000,"maximum number of components");
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
            int n = label_components(segmentation);
            if(n>pgetf("max_n")) throw "too many connected components";
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

    struct StandardPreprocessing : virtual IBinarize,virtual ICleanupGray,virtual ICleanupBinary {
        autodel<ICleanupGray> graydeskew;
        autodel<ICleanupBinary> bindeskew;
        autodel<IBinarize> binarizer;
        narray< autodel<ICleanupGray> > grayclean;
        narray< autodel<ICleanupBinary> > binclean;
        const char *interface() {
            return "IBinarize";
        }
        const char *name() {
            return "preproc";
        }
        StandardPreprocessing() {
            pdef("grayclean0","","grayscale page cleanup");
            pdef("grayclean1","","grayscale page cleanup");
            pdef("grayclean2","","grayscale page cleanup");
            pdef("grayclean3","","grayscale page cleanup");
            pdef("grayclean4","","grayscale page cleanup");
            pdef("grayclean5","","grayscale page cleanup");
            pdef("grayclean6","","grayscale page cleanup");
            pdef("grayclean7","","grayscale page cleanup");
            pdef("grayclean8","","grayscale page cleanup");
            pdef("grayclean9","","grayscale page cleanup");
            pdef("graydeskew","DeskewPageByRAST","grayscale deskewing");
            pdef("binarizer","BinarizeBySauvola","binarizer used for pages");
            pdef("binclean0","AutoInvert","binary page cleanup");
            pdef("binclean1","RmHalftone","binary page cleanup");
            pdef("binclean2","RmBig","binary page cleanup");
            pdef("binclean3","","binary page cleanup");
            pdef("binclean4","","binary page cleanup");
            pdef("binclean5","","binary page cleanup");
            pdef("binclean6","","binary page cleanup");
            pdef("binclean7","","binary page cleanup");
            pdef("binclean8","","binary page cleanup");
            pdef("binclean9","","binary page cleanup");
            pdef("bindeskew","DeskewPageByRAST","binary deskewing");
            reinit();
            persist(graydeskew,"graydeskew");
            persist(binarizer,"binarizer");
            persist(graydeskew,"bindeskew");
            for(int i=0;i<10;i++) {
                char buf[100];
                sprintf(buf,"grayclean%d",i);
                persist(grayclean[i],buf);
                sprintf(buf,"binclean%d",i);
                persist(binclean[i],buf);
            }
        }
        const char *command(const char *argv[]) {
            if(!strcmp(argv[0],"init")) {
                reinit();
                return "OK";
            }
            if(!strcmp(argv[0],"binarizer_pset")) {
                CHECK_ARG(argv[1] && argv[2]);
                if((argv[2][0]>='0' && argv[2][0]<='9') || argv[2][0]=='-') {
                    binarizer->pset(argv[1],atof(argv[2]));
                } else {
                    binarizer->pset(argv[1],argv[2]);
                }
                return argv[2];
            }
            return 0;
        }
        void reinit() {
            make_component(binarizer,pget("binarizer"));
            make_component(bindeskew,pget("bindeskew"));
            make_component(graydeskew,pget("graydeskew"));
            // pprint();
            grayclean.resize(10);
            binclean.resize(10);
            for(int i=0;i<10;i++) {
                strg s;
                sprintf(s,"grayclean%d",i);
                const char *key;
                key = pget(s);
                if(key && strcmp(key,""))
                    make_component(grayclean[i],pget(s));
                sprintf(s,"binclean%d",i);
                key = pget(s);
                if(key && strcmp(key,""))
                    make_component(binclean[i],pget(s));
            }
        }
        void cleanup_gray(bytearray &out,bytearray &in) {
            bytearray temp;
            out = in;
            for(int i=0;i<grayclean.length();i++) {
                if(!grayclean[i]) continue;
                try {
                    grayclean[i]->cleanup_gray(temp,out);
                    out.move(temp);
                } catch(const char *s) {
                    debugf("warn","grayclean%d failed: %s\n",i,s);
                }
            }
        }
        void cleanup(bytearray &out,bytearray &in) {
            bytearray temp;
            out = in;
            for(int i=0;i<binclean.length();i++) {
                if(!binclean[i]) continue;
                try {
                    binclean[i]->cleanup(temp,out);
                    out.move(temp);
                } catch(const char *s) {
                    debugf("warn","binclean%d failed: %s\n",i,s);
                }
            }
        }
        void binarize(bytearray &out,bytearray &in) {
            bytearray gray;
            binarize(out,gray,in);
        }
        void binarize(bytearray &out,bytearray &gray,bytearray &in) {
            if(contains_only(in,0,255)) {
                bytearray temp;
                cleanup(out,in);
                if(bindeskew) {
                    temp.move(out);
                    try {
                        bindeskew->cleanup(out,temp);
                    } catch(const char *s) {
                        debugf("warn","graydeskew failed: %s\n",s);
                        // just continue as if nothing happened
                        out.move(temp);
                    }
                    gray = out;
                }
            } else {
                bool deskewed = 0;
                bytearray temp;
                cleanup_gray(out,in);
                if(graydeskew) {
                    temp.move(out);
                    try {
                        graydeskew->cleanup_gray(out,temp);
                    } catch(const char *s) {
                        debugf("warn","graydeskew failed: %s\n",s);
                        // just continue as if nothing happened
                        out.move(temp);
                    }
                    deskewed = 1;
                    gray = out;
                }
                temp.move(out);
                try {
                    binarizer->binarize(out,temp);
                    temp.move(out);
                } catch(const char *s) {
                    debugf("warn","binarizer failed: %s\n",s);
                    // just continue as if nothing happened
                }
                cleanup(out,temp);
                if(!deskewed && bindeskew) {
                    temp.move(out);
                    try {
                        bindeskew->cleanup(out,temp);
                    } catch(const char *s) {
                        debugf("warn","bindeskew failed: %s\n",s);
                        // just continue as if nothing happened
                        out.move(temp);
                    }
                }
            }
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

    IBinarize *make_StandardPreprocessing() {
        return new StandardPreprocessing();
    }
}
