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
// Project: ocropus
// File: ocr-orientation.cc
// Purpose: find orientation of a page, based on OCR
// Responsible: Rangoni Yves
// Reviewer:
// Primary Repository:
// Web Sites: www.iupr.org, www.dfki.de

#include <wctype.h>
#include "ocr-orientation.h"
#include "ocropus.h"
#include "omp.h"

using namespace ocropus;

Logger logger("ocr-orientation");

/*bool is_equal_lower_upper(nuchar &c1, nuchar &c2) {
    return towlower((wchar_t)(c1.ord())) == towlower((wchar_t)(c1.ord()));
}

bool is_equal_lower_upper(nustring &s1, nustring &s2) {
    if (s1.length() != s2.length())
        return false;
    for (int i=0; i<s1.length(); i++)
        if (!is_equal_lower_upper(s1[i],s2[i]))
            return false;
    return true;
}*/

void tonslower(nustring &s1, nustring &s2) {
    s1.clear();
    for (int i=0; i < s2.length(); ++i)
        s1.push(nuchar( towlower((wchar_t)(s2(i).ord())) ));
}

void tonslower(nustring &s1) {
    for (int i=0; i < s1.length(); ++i)
        s1(i) = nuchar( towlower((wchar_t)(s1(i).ord())) );        
}

void rotate_page(bytearray &out, bytearray &in, int orientation) {
    switch (orientation) {
        case 0:  copy(out, in);  break;
        case 90: rotate_90(out, in); break;
        case 180:rotate_180(out, in); break;
        case 270:rotate_270(out, in); break;
        default: exit(0);
    }
}

void construct_dict(objlist< nustring > &dict, nustring &alphabet, const char* dictfile) {
    FILE* f = fopen(dictfile, "rt");
    ASSERT(f!=0);
    char tempbuf[1024];
    dict.clear();
    alphabet.clear();

    while (fgets(tempbuf, 1024, f)) {               // assumes that the dictionary is a list of words in column
        tempbuf[max(0, strlen(tempbuf)-1)] = '\0';  // remove trailing \n        
        nustring word(tempbuf);
        tonslower(word);
        if (word.length() > 1) {                       // we want at least 2 characters in a word
            copy(dict.push(), word);
            for (int i=0; i < word.length(); i++) 
                if (first_index_of(alphabet, word[i]) == -1)
                    alphabet.push(word[i]);
        }
    }
    fclose(f);
}
        
namespace ocropus {
    const char* ocrorientation::description() {
        return "An OCR-based page orientation detection \
                It looks for best orientation if the 4 directions according \
                to the transcription quality on a subset of lines\n";
    }

    const char* ocrorientation::name() {
        return "ocrorientation";
    }  
        
    ocrorientation::ocrorientation(const char* dictfile, int m_l_c, int m_l_u, const char* m) {
        max_lines_to_compute = max(m_l_c,1);
        max_lines_to_use = max(m_l_u,1);
        mode = strdup(m);
        angle = -1;
        in_image.resize(0);
        out_image.resize(0);
        
        construct_dict(dict, alphabet, dictfile);
        
        logger.log("Alphabet", alphabet);
    }
    
    ocrorientation::~ocrorientation() {
        dict.dealloc();
        if (mode) free(mode);
    }
    
    //it supposes that: dict is in lower case as well as word
    bool ocrorientation::is_in_dict(nustring &word) {
        for (int i=0; i<dict.length(); i++)
            if (word == dict(i))
                return true;
        return false;
    }
    
    void ocrorientation::extractwords(objlist< nustring > &tokens, nustring &s) {
        nustring word;
        nustring string;
        tonslower(string, s);
        for (int i=0; i<string.length(); i++)
            if (first_index_of(alphabet, string[i]) != -1)
                word.push(string[i]);
            else {
                if (word.length() > 1)
                    copy(tokens.push(), word);
                word.clear();
            }

        if (word.length() > 1)
            copy(tokens.push(), word);
    }
    
    float ocrorientation::score_line(nustring &input) {
        int n = input.length();
        if (n == 0) return -1;
        if (n < 10) return -2;
        objlist<nustring> tokens;
        extractwords(tokens, input);
        
        float c = 0.;
        for (int i=0; i<tokens.length(); i++)
            if (is_in_dict(tokens(i)))
                c += 1+tokens(i).length();
                
        if (c < 8)  return -3;
        return c/(n+1);
    }
    
    void ocrorientation::add_image(bytearray &in) {
        copy(in_image, in);        
        angle = -1;
    }
    
    void ocrorientation::get_image(bytearray &out) {
        get_angle();
        if (angle != -1)
        rotate_page(out, in_image, angle);
        else
            copy(out, in_image);
    }
    
    int ocrorientation::get_angle() {
        if (angle == -1)
            estimate_all_orientations();
        return angle;
    }
    
    void ocrorientation::estimate_one_orientation_with_tess(floatarray &list_scores, bytearray &page_image, int orientation, int maxlines) {
        bytearray page_image_rot;
        intarray  page_segmentation;

        logger.log("orientation", orientation);
        rotate_page(page_image_rot, page_image, orientation);
        
        list_scores.clear();
        list_scores.push(-4.);
        
        RegionExtractor regions;
        autodel<ISegmentPage> segmenter(make_SegmentPageByRAST());
        segmenter->set("max_results", max_lines_to_compute);
        
        segmenter->segment(page_segmentation, page_image_rot);
        logger.recolor("segmentation", page_segmentation, 20);
        regions.setPageLines(page_segmentation);
        logger.log("region length",regions.length());

        if (regions.length() > 1) {
            autodel<IRecognizeLine> tesseract_recognizer(make_TesseractRecognizeLine());
            
            intarray bb_len;
            bb_len.resize(regions.length()-1);
            intarray permut;
            permut.resize(regions.length()-1);
            for (int i=1;i<regions.length();i++) {
                permut[i-1] = i;
                bb_len[i-1] = -regions.bbox(i).width();
            }
            logger.log("bb widths", bb_len);
            quicksort(permut, bb_len, 0, regions.length()-2);
            //permute(bb_len, permut);
            logger.log("permute", permut);
            bytearray line_image;
            nustring transcript;
            
            for (int j=0;j<min(permut.length(), maxlines);j++) {
                regions.extract(line_image, page_image_rot, permut[j], 1);
                remove_neighbour_line_components(line_image);
                logger.format("image for orientation: %d and trial %d",orientation, j);
                logger.log("cleaned line", line_image);
                
                autodel<IGenericFst> fst(make_OcroFST());
                tesseract_recognizer->recognizeLine(*fst, line_image); 
                
                nustring result;
                fst->bestpath(result);
                
                float score = score_line(result);
                logger.log("score", score);
                logger.log("transcript", result);

                list_scores.push(score);
            }
        }
    }
    
    void ocrorientation::estimate_all_orientations() {
        floatarray scores;
        scores.resize(4);
        
        logger.log("getGray image", in_image, 20.);
        
        autodel<IBinarize> binarize(make_BinarizeBySauvola());
        binarize->binarize(in_image_binarized, in_image);
        logger.log("getBinary image", in_image_binarized, 20.);
        
        
#pragma omp parallel for num_threads(4)
        for (int a=0; a<4; a++) {
            floatarray list_scores;
            estimate_one_orientation_with_tess(list_scores, in_image_binarized, a*90, max_lines_to_compute);
            if (strcmp(mode, "max") == 0) {
                int argmax = 0;
                for (int j=1; j <= min(max_lines_to_use, list_scores.length()); j++) 
                    if (list_scores[j] > list_scores[argmax])
                        argmax = j;
                scores[a] = list_scores[argmax];
            } else if (strcmp(mode, "mean")) {
                float sum = 0;
                for (int j=0; j <= min(max_lines_to_use, list_scores.length()); j++)
                    if (list_scores[j] > 0)
                        sum += list_scores[j];
                scores[a] = sum/max(max_lines_to_use, list_scores.length());
            } else {
                exit(1);
            }
        }
        logger.format("orientation scores (0,90,180,270)°: %f %f %f %f",scores[0],scores[1],scores[2],scores[3]);
        angle = argmax(scores)*90;
    }
}    

