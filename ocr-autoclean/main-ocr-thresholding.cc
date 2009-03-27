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
// File: main-ocr-thresholding.cc
// Purpose: find best Sauvola's parameters for a page, based on OCR
// Responsible: Rangoni Yves
// Reviewer:
// Primary Repository:
// Web Sites: www.iupr.org, www.dfki.de

#include "ocropus.h"
#include "ocr-thresholding.h"

using namespace colib;
using namespace iulib;
using namespace ocropus;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s [dictionary_file] [in_image]\n",argv[0]);
        printf("Usage: %s [dictionary_file] [in_image] [out_image]\n",argv[0]);
        exit(-1);
    }
    ocrthresholding oo(argv[1]);
    bytearray in, out;
    read_image_gray(in, argv[2]);
    tesseract_init_with_language("dum");
    oo.add_image(in);
    if (argc == 4) {
        oo.get_image(out);
        write_image_gray(argv[3], out);
    } else {
        int w;
        float k;
        oo.get_parameters(w, k);
        printf("%d %f\n", w, k);
    }
    return 0;
}
