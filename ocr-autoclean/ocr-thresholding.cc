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
// File: ocr-thresholding.cc
// Purpose: find best Sauvola's parameters for a page, based on OCR
// Responsible: Rangoni Yves
// Reviewer:
// Primary Repository:
// Web Sites: www.iupr.org, www.dfki.de

#include <wctype.h>
#include "ocr-thresholding.h"
#include "ocropus.h"
#include <omp.h>


using namespace ocropus;
using namespace iulib;
using namespace colib;

Logger logger("ocr-thresholding");

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
    for (int i = 0; i < s2.length(); ++i)
        s1.push(nuchar( towlower((wchar_t)(s2(i).ord())) ));
}

void tonslower(nustring &s1) {
    for (int i = 0; i < s1.length(); ++i)
        s1(i) = nuchar( towlower((wchar_t)(s1(i).ord())) );
}

void construct_dict(objlist< nustring > &dict, nustring &alphabet,
    const char* dictfile) {
    FILE* f = fopen(dictfile, "rt");
    ASSERT(f != NULL);
    char tempbuf[1024];
    dict.clear();
    alphabet.clear();

    while (fgets(tempbuf, 1024, f)) {                                           // assumes that the dictionary is a list of words in column
        tempbuf[max(0, strlen(tempbuf)-1)] = '\0';                              // remove trailing \n
        nustring word(tempbuf);
        tonslower(word);                                                        // in lower case
        if (word.length() > 1) {                                                // we want at least 2 characters in a word
            copy(dict.push(), word);
            for (int i = 0; i < word.length(); i++)
                if (first_index_of(alphabet, word[i]) == -1)                    // if a new letter is coming
                    alphabet.push(word[i]);                                     // ad it to the alphabet
        }
    }
    fclose(f);
}

// TODO: wait the new string library from Mathias and do not use these string manipulations

namespace ocropus {
    const char* ocrthresholding::description() {
        return "An OCR-based thresholding optimizer \
                It looks for best Sauvola's parameters (W,K) according \
                to the transcription quality on a subset of lines\n";
    }

    const char* ocrthresholding::name() {
        return "ocrthresholding";
    }

    ocrthresholding::ocrthresholding(const char* dictfile, int m_l_c, int m_l_u,
        const char* m) {
        max_lines_to_compute = max(m_l_c, 1);
        max_lines_to_use = max(m_l_u, 1);
        mode = strdup(m);
        ASSERT(mode != NULL);
        W = -1;
        K = -1;
        in_image.resize(0);
        out_image.resize(0);
        construct_dict(dict, alphabet, dictfile);                               // most important part: pre compute dictionary
        logger.log("Alphabet", alphabet);                                       // and the alphabet to be able to extract tokens
    }

    ocrthresholding::~ocrthresholding() {
        dict.dealloc();
        if (mode) free(mode);
    }

    //it supposes that: dict is in lower case as well as word
    bool ocrthresholding::is_in_dict(nustring &word) {
        for (int i = 0; i < dict.length(); i++)                                 // can be optimized by sorting the dictionary
            if (word == dict(i))
                return true;
        return false;
    }

    void ocrthresholding::extractwords(objlist< nustring > &toks, nustring &s) {
        nustring word;
        nustring string;
        tonslower(string, s);                                                   // in lower case, cause the dict is in lower case
        for (int i = 0; i < string.length(); i++)
            if (first_index_of(alphabet, string[i]) != -1)                      // a letter?
                word.push(string[i]);                                           // append to the current word
            else {                                                              // end of a word
                if (word.length() > 1)                                          // only words of 2 chars at least
                    copy(toks.push(), word);                                    // ok: it's a token
                word.clear();                                                   // clear the word
            }

        if (word.length() > 1)                                                  // last word
            copy(toks.push(), word);                                            // add to the list of tokens if at least 2 chars
    }

    float ocrthresholding::score_line(nustring &input) {
        int n = input.length();
        if (n == 0) return -1;
        if (n < 10) return -2;
        objlist< nustring > tokens;
        extractwords(tokens, input);

        float c = 0.;
        for (int i = 0; i < tokens.length(); i++)
            if (is_in_dict(tokens(i)))                                          // for all the words in dict
                c += 1 + tokens(i).length();                                    // add the words length +1

        if (c < 8)  return -3;                                                  // to few valid chars
        return c / (n + 1);                                                     // ratio of good chars vs. string length
    }

    void ocrthresholding::add_image(bytearray &in) {
        copy(in_image, in);
        W = -1;
        K = -1;
    }

    void ocrthresholding::get_image(bytearray &out) {
        get_parameters(W, K);                                                   // search
        autodel<IBinarize> binarize(make_BinarizeBySauvola());
        if (W != -1) {                                                          // if we found interesting parameters
            binarize->set("w", W);                                              // use them
            binarize->set("k", K);
        }                                                                       // otherwise, take default values
        binarize->binarize(out, in_image);                                      // binarize
    }

    void ocrthresholding::get_parameters(int &w, float &k) {
        if ((W == -1) || (K == -1))                                             // no need to search if we have the right parameters
            estimate_all_thresholdings();
        w = W;
        k = K;
    }

    // evaluate a binarized line
    float ocrthresholding::evaluate_binarization(bytearray &line_image_binarized) {
        remove_neighbour_line_components(line_image_binarized);                 // maybe not necessary
        logger.log("line_image binarized cleaned", line_image_binarized);       // log

        autodel<IGenericFst> fst(make_OcroFST());                               // prepare FST
        autodel<IRecognizeLine>
            tesseract_recognizer(make_TesseractRecognizeLine());                // a Tesseract line recognizer

        tesseract_recognizer->recognizeLine(*fst, line_image_binarized);        // recognize
        nustring result;                                                        // prepare transcription
        fst->bestpath(result);                                                  // get transcription

        float score = score_line(result);                                       // get score

        logger.log("score", score);                                             // log
        logger.log("transcript", result);
        return score;
    }

    float ocrthresholding::evaluate_binarization(
        bytearray &line_image_grey, int w, float k) {

        bytearray line_image_binarized;                                         // prepare binarized image
        autodel<IBinarize> binarizer(make_BinarizeBySauvola());                 // prepare Sauvola
        binarizer->set("w", w);                                                 // set Sauvola's parameters
        binarizer->set("k", k);
        binarizer->binarize(line_image_binarized, line_image_grey);             // apply Sauvola
        // the grey image was wider to apply Sauvola with w
        // crop to obtain the real line and remove its neighborhood
        // but keep one extra pixel to prevent some drawbacks of remove_neighbour_line_components
        crop(line_image_binarized,                                              // crop to the right region: we extended to
            rectangle(w/2, w/2,                                                 // apply carefuly Sauvola
                line_image_grey.dim(0)-w/2+2,
                line_image_grey.dim(1)-w/2+2));
        logger.log("line_image binarized", line_image_binarized);               // log
        return evaluate_binarization(line_image_binarized);                     // get the score
    }

    void ocrthresholding::estimate_all_thresholdings() {
        logger.log("in_image", in_image, 20.);

        bytearray page_image_binarized;
        autodel<IBinarize> binarize(make_BinarizeBySauvola());                  // after experiment, (60,0.4) is in average
        binarize->set("w", 60);                                                 // suitable, but to be sure to not produce to small
        binarize->set("k", 0.3);                                                // bbox, maybe (60,0.3) is better
        binarize->binarize(page_image_binarized, in_image);
        pad_by(page_image_binarized, 90/2, 90/2, colib::byte(255));             // a pad to avoid some strange cases where a line touches a border
        pad_by(in_image, 90/2, 90/2, colib::byte(255));                         // but it is a "long" job (waste of time?) for few degenerate cases...
        logger.log("initial binarization", page_image_binarized, 20.);
        RegionExtractor regions;
        autodel<ISegmentPage> segmenter(make_SegmentPageByRAST());
        segmenter->set("max_results", max_lines_to_compute);                    // do not extract all the lines for nothing
        intarray  page_segmentation;
        segmenter->segment(page_segmentation, page_image_binarized);            // segment
        logger.recolor("segmentation", page_segmentation, 20.);
        regions.setPageLines(page_segmentation);                                // get regions
        logger.log("region length",regions.length());

        float   best_sumscores = -1;
        int     best_W = -1;
        float   best_K = -1;
        if (regions.length() > 1) {
            autodel<IRecognizeLine> tesseract_recognizer(                       // TODO any kind of line recognizer can work
                make_TesseractRecognizeLine());
            intarray bb_len;
            bb_len.resize(regions.length() - 1);                                // first region is not a line
            intarray permut;
            permut.resize(regions.length() - 1);
            for (int i = 1; i < regions.length(); i++) {
                permut[i - 1] = i - 1;                                          // beware of quicksort behaviour, index starts at 0
                bb_len[i - 1] = -regions.bbox(i).width();
            }

            logger.log("bb widths", bb_len);
            quicksort(permut, bb_len, 0, permut.length());                      // wider lines first
            logger.log("permute", permut);

            bytearray line_image_gray;

            for (int w = 10; w < 91;w += 10) {
                for (float k = 0.1;k < 0.91;k += 0.1) {                         // test <> parameters
                    printf("%d %g  \r", w, k);fflush(stdout);
                    float score, sumscores = 0;
                    logger.format("try (w,k)=(%d,%f)", w, k);
                    for (int j=0;j<min(permut.length(), max_lines_to_use);j++) {
                        regions.extract(line_image_gray, in_image, permut[j]+1, //because of quicksort behaviour
                            w/2+1);
                        logger.log("line_image_gray", line_image_gray);

                        score = evaluate_binarization(line_image_gray, w, k);   // evaluate this couple (w,k)

                        if (score > 0)
                            sumscores += score;                                 // add the result
                    }
                    logger.format("score (w,k)=(%d,%f) %f", w, k, sumscores);
                    if (sumscores > best_sumscores) {                           // if we improve
                        best_sumscores = sumscores;                             // take it
                        best_W = w;
                        best_K = k;
                    }
                }
            }
        }
        logger.format("best (w,k)=(%d,%f) with score=%f", best_W, best_K, best_sumscores);
        W = best_W;                                                             // best (W,K) or (-1,-1) if something went wrong
        K = best_K;
    }
    
    ocrthresholding *make_OCRAutoThresholding(const char* dict) {
        return new ocrthresholding(dict);
    }
}

