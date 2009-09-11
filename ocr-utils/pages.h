// -*- C++ -*-

// Copyright 2006-2007 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz
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
// File:
// Purpose:
// Responsible: tmb
// Reviewer:
// Primary Repository:
// Web Sites: www.iupr.org, www.dfki.de

#ifndef h_pages__
#define h_pages__

#include "ocropus.h"

namespace ocropus {

    struct Pages : IComponent {
        autodel<IBinarize> binarizer;
        narray<strg> files;
        intarray numSubpages;    /// number of (sub)pages within each image
        bool want_gray;
        bool want_color;

        int current_index;    /// overall index of current page
        int current_image;    /// index of current image
        int current_subpage;  /// index of page within current image

        bool has_gray;
        bool has_color;
        bool autoinv;
        bytearray binary;
        bytearray gray;
        intarray color;

        Pages() {
            rewind();
            autoinv = 1;
            pdef("binarizer","StandardPreprocessing","binarizer used for pages");
            make_component(binarizer,pget("binarizer"));
        }

        const char *interface() { return "Pages"; }

        void info(int depth,FILE *stream) {
            iprintf(stream,depth,"Pages data structure");
            pprint(stream,depth);
        }
        const char *name() {
            return "pages";
        }
        void clear() {
            files.clear();
        }
        static bool isTiff(strg& filename) {
            return re_search(filename, "\\.tif\\(f\\?\\)$") >= 0;
        }
        void addFile(const char *file) {
            files.push() = file;
            numSubpages.push(isTiff(files.last())?Tiff(file, "r").numPages():1);
        }
        void parseSpec(const char *spec) {
            current_index = -1;
            clear();
            if(spec[0]=='@') {
                char buf[9999];
                stdio stream(spec+1,"r");
                for(;;) {
                    if(!fgets(buf,sizeof buf,stream)) break;
                    int n = strlen(buf);
                    if(n>0) buf[n-1] = 0;
                    addFile(buf);
                }
            } else {
                addFile(spec);
            }
        }
        void wantGray(bool flag) {
            want_gray = flag;
        }
        void wantColor(bool flag) {
            want_color =flag;
        }
        void setAutoInvert(bool flag) {
            autoinv = flag;
        }
        void setBinarizer(IBinarize *arg) {
            binarizer = arg;
        }
        int length() {
            return files.length();
        }
        void getPage(int index) {
            current_index = 0;
            current_image = 0;
            while(current_index + numSubpages(current_image) <= index) {
                current_index += numSubpages(current_image);
                current_image++;
            }
            current_subpage = index - current_index;
            current_index = index;
            loadImage();
        }
        bool nextPage() {
            if(current_image>=files.length()) return false;
            current_index++;
            current_subpage++;
            if(current_image < 0 || current_subpage >= numSubpages(current_image)) {
                current_subpage = 0;
                current_image++;
            }
            if(current_image>=files.length()) return false;
            loadImage();
            return true;
        }
        void rewind() {
            current_index = -1;
            current_image = -1;
            current_subpage = -1;
        }
        void loadImage() {
            has_gray = false;
            has_color = false;
            binary.clear();
            gray.clear();
            color.clear();
            strg& current_file = files(current_image);
            if(current_subpage > 0) {
                if(!isTiff(current_file)) {
                    throw "subpage requested but not a TIFF image";
                }
                Tiff(current_file, "r").getPage(gray, current_subpage);
            } else {
                iulib::read_image_gray(gray,current_file);
            }
            if(autoinv) {
                iulib::make_page_black(gray);
                invert(gray);
            }
            if(!binarizer) {
                float v0 = min(gray);
                float v1 = max(gray);
                float threshold = (v1+v0)/2;
                makelike(binary,gray);
                for(int i=0;i<gray.length1d();i++)
                    binary.at1d(i) = (gray.at1d(i) > threshold) ? 255:0;
            } else {
                binarizer->binarize(binary,gray);
            }
        }
        const char *getFileName() {
            return files(current_image).c_str();
        }
        bool hasGray() {
            return true;
        }
        bool hasColor() {
            return false;
        }
        bytearray &getBinary() {
            return binary;
        }
        bytearray &getGray() {
            return gray;
        }
        bytearray &getColor() {
            throw "unimplemented";
        }
        void getBinary(bytearray &dst) {
            copy(dst,binary);
        }
        void getGray(bytearray &dst) {
            copy(dst,gray);
        }
        void getColor(intarray &dst) {
            copy(dst,color);
        }
    private:
        //FIXME already in ocr-utils/docproc.h / ocr-utils/ocr-utils.cc
        //      can it be removed? --remat
        void invert(bytearray &a) {
            int n = a.length1d();
            for (int i = 0; i < n; i++) {
                a.at1d(i) = 255 - a.at1d(i);
            }
        }
    };
}

#endif
