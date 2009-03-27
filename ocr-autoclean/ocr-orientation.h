// Copyright 2006-2009 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz
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
// Project: ocropus
// File: ocr-orientation.h
// Purpose: find orientation of a page, based on OCR
// Responsible: Rangoni Yves
// Reviewer:
// Primary Repository:
// Web Sites: www.iupr.org, www.dfki.de

#ifndef h_ocr_orientation
#define h_ocr_orientation

#include "ocropus.h"

using namespace colib;

namespace ocropus {

class ocrorientation : IComponent {
    public:
        int max_lines_to_compute;
        int max_lines_to_use;
        char* mode;

                ocrorientation( const char* dictionary_file,
                                int max_lines_to_compute = 3,
                                int max_lines_to_use = 3,
                                const char* mode = "max");
                ~ocrorientation();

        void    add_image(bytearray &in);
        void    get_image(bytearray &out);
        int     get_angle();

        const char* description();
        const char* name();

    private:
        objlist< nustring >  dict;
        bytearray   in_image;
        bytearray   out_image;

        nustring    alphabet;
        int         angle;

        bool    is_in_dict(nustring &s);
        void    extractwords(objlist< nustring > &tokens, nustring &s);
        float   score_line(nustring &line);
        void    estimate_one_orientation_with_tess(floatarray &list_scores, bytearray &page_image, int orientation);
        void    estimate_all_orientations();
    };
    
    ocrorientation *make_OCRAutoOrientation(const char* dict);
}

#endif
