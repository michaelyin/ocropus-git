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
// Purpose: An efficient implementation of the Sauvola's document binarization
//          algorithm based on integral images as described in
//          F. Shafait, D. Keysers, T.M. Breuel. "Efficient Implementation of
//          Local Adaptive Thresholding Techniques Using Integral Images".
//          Document Recognition and Retrieval XV, San Jose.
//
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

    param_string debug_binarize("debug_binarize",0,"output the result of binarization");

    struct BinarizeBySauvola : IBinarize {
        float k;
        int w;
        int whalf; // Half of window size

        BinarizeBySauvola() {
            pdef("k",0.3,"Weighting factor");
            pdef("w",40,"Local window size. Should always be positive");
        }

        ~BinarizeBySauvola() {}

        const char *description() {
            return "An efficient implementation of the Sauvola's document \
binarization algorithm based on integral images.\n";
        }

        const char *name() {
            return "binsauvola";
        }

        void binarize(bytearray &out, floatarray &in){
            bytearray image;
            copy(image,in);
            binarize(out,image);
        }

        void binarize(bytearray &bin_image, bytearray &gray_image){
            w = pgetf("w");
            k = pgetf("k");
            whalf = w>>1;
            // fprintf(stderr,"[sauvola %g %d]\n",k,w);
            CHECK_ARG(k>=0.001 && k<=0.999);
            CHECK_ARG(w>0 && k<1000);
            if(bin_image.length1d()!=gray_image.length1d())
                makelike(bin_image,gray_image);

            if(contains_only(gray_image,byte(0),byte(255))){
                copy(bin_image,gray_image);
                return ;
            }

            int image_width  = gray_image.dim(0);
            int image_height = gray_image.dim(1);
            whalf = w>>1;

            // Calculate the integral image, and integral of the squared image
            narray<int64_t> integral_image,rowsum_image;
            narray<int64_t> integral_sqimg,rowsum_sqimg;
            makelike(integral_image,gray_image);
            makelike(rowsum_image,gray_image);
            makelike(integral_sqimg,gray_image);
            makelike(rowsum_sqimg,gray_image);
            int xmin,ymin,xmax,ymax;
            double diagsum,idiagsum,diff,sqdiagsum,sqidiagsum,sqdiff,area;
            double mean,std,threshold;

            for(int j=0; j<image_height; j++){
                rowsum_image(0,j) = gray_image(0,j);
                rowsum_sqimg(0,j) = gray_image(0,j)*gray_image(0,j);
            }
            for(int i=1; i<image_width; i++){
                for(int j=0; j<image_height; j++){
                    rowsum_image(i,j) = rowsum_image(i-1,j) + gray_image(i,j);
                    rowsum_sqimg(i,j) = rowsum_sqimg(i-1,j) + gray_image(i,j)*gray_image(i,j);
                }
            }

            for(int i=0; i<image_width; i++){
                integral_image(i,0) = rowsum_image(i,0);
                integral_sqimg(i,0) = rowsum_sqimg(i,0);
            }
            for(int i=0; i<image_width; i++){
                for(int j=1; j<image_height; j++){
                    integral_image(i,j) = integral_image(i,j-1) + rowsum_image(i,j);
                    integral_sqimg(i,j) = integral_sqimg(i,j-1) + rowsum_sqimg(i,j);
                }
            }

            //Calculate the mean and standard deviation using the integral image

            for(int i=0; i<image_width; i++){
                for(int j=0; j<image_height; j++){
                    xmin = max(0,i-whalf);
                    ymin = max(0,j-whalf);
                    xmax = min(image_width-1,i+whalf);
                    ymax = min(image_height-1,j+whalf);
                    area = (xmax-xmin+1)*(ymax-ymin+1);
                    // area can't be 0 here
                    // proof (assuming whalf >= 0):
                    // we'll prove that (xmax-xmin+1) > 0,
                    // (ymax-ymin+1) is analogous
                    // It's the same as to prove: xmax >= xmin
                    // image_width - 1 >= 0         since image_width > i >= 0
                    // i + whalf >= 0               since i >= 0, whalf >= 0
                    // i + whalf >= i - whalf       since whalf >= 0
                    // image_width - 1 >= i - whalf since image_width > i
                    // --IM
                    ASSERT(area);
                    if(!xmin && !ymin){ // Point at origin
                        diff   = integral_image(xmax,ymax);
                        sqdiff = integral_sqimg(xmax,ymax);
                    }
                    else if(!xmin && ymin){ // first column
                        diff   = integral_image(xmax,ymax) - integral_image(xmax,ymin-1);
                        sqdiff = integral_sqimg(xmax,ymax) - integral_sqimg(xmax,ymin-1);
                    }
                    else if(xmin && !ymin){ // first row
                        diff   = integral_image(xmax,ymax) - integral_image(xmin-1,ymax);
                        sqdiff = integral_sqimg(xmax,ymax) - integral_sqimg(xmin-1,ymax);
                    }
                    else{ // rest of the image
                        diagsum    = integral_image(xmax,ymax) + integral_image(xmin-1,ymin-1);
                        idiagsum   = integral_image(xmax,ymin-1) + integral_image(xmin-1,ymax);
                        diff       = diagsum - idiagsum;
                        sqdiagsum  = integral_sqimg(xmax,ymax) + integral_sqimg(xmin-1,ymin-1);
                        sqidiagsum = integral_sqimg(xmax,ymin-1) + integral_sqimg(xmin-1,ymax);
                        sqdiff     = sqdiagsum - sqidiagsum;
                    }

                    mean = diff/area;
                    std  = sqrt((sqdiff - diff*diff/area)/(area-1));
                    threshold = mean*(1+k*((std/128)-1));
                    if(gray_image(i,j) < threshold)
                        bin_image(i,j) = 0;
                    else
                        bin_image(i,j) = MAXVAL-1;
                }
            }
            if(debug_binarize) {
                write_png(stdio(debug_binarize, "w"), bin_image);
            }
        }

    };

    IBinarize *make_BinarizeBySauvola() {
        return new BinarizeBySauvola();
    }

    struct BinarizeByHT : IBinarize {
        p_float k0;
        p_float k1;
        p_float width;
        p_int max_n;

        BinarizeByHT() {
            max_n.bind(this,"max_n",5000,"maximum number of connected components");
            k0.bind(this,"k0",0.2,"low threshold");
            k1.bind(this,"k1",0.6,"high threshold");
            width.bind(this,"width",40.0,"width of region");
        }

        ~BinarizeByHT() {}

        const char *description() {
            return "binarization by hysteresis thresholding";
        }

        const char *name() {
            return "binht";
        }

        void binarize(bytearray &out, floatarray &in){
            bytearray image;
            copy(image,in);
            binarize(out,image);
        }

        void binarize(bytearray &bin_image, bytearray &image){
            dsection("binht");
            using namespace narray_ops;
            autodel<IBinarize> threshold;
            make_component(threshold,"BinarizeBySauvola");
            threshold->pset("w",width);
            bytearray &image0 = bin_image;
            bytearray image1;
            threshold->pset("k",k0);
            threshold->binarize(image0,image);
            threshold->pset("k",k1);
            threshold->binarize(image1,image);
            sub(max(image0),image0);
            sub(max(image1),image1);
            dshown(image0,"a");
            dshown(image1,"b");
            intarray blobs;
            blobs = image0;
            int n = label_components(blobs);
            if(n>max_n) throw "too many connected components";
            intarray flags(n+1);
            flags = 0;
            for(int i=0;i<blobs.length();i++) {
                if(!image1[i]) continue;
                flags[blobs[i]] = 1;
            }
            debugf("debug","retained %d of %d blobs\n",int(sum(flags)),flags.length());
            for(int i=0;i<image0.length();i++)
                if(!flags[blobs[i]])
                    image0[i] = 0;
            for(int i=0;i<image0.length();i++)
                image0[i] = 255*!image0[i];
        }
    };

    IBinarize *make_BinarizeByHT() {
        return new BinarizeByHT();
    }
} //namespace
