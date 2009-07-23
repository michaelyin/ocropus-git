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
// Project: ocrpus extras
// File: pageseg.cc
// Purpose: training of archive.org books
// Responsible: remat
// Reviewer:
// Primary Repository:
// Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org

#include <colib.h>
#include <iulib.h>
#include <iustring.h>
#include <ocropus.h>
#include <ocr-layout.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace colib;
using namespace iulib;
using namespace ocropus;

typedef struct {
    rectangle bbox;
    rectarray charBBoxes;
    ustrg trans;
} Line;

void renumber(intarray& src) {
    intarray dst;
    dst.makelike(src);
    inthash<int> k;
    int c = 1;
    for(int i=0; i<src.length1d(); i++) {
        if(((src.at1d(i) >> 8) & 0xFFFF) == 0xFFFF) {
            dst.at1d(i) = 0;
        } else {
            if(k.find(src.at1d(i)) == 0) {
                k(src.at1d(i)) = c++;
            }
            dst.at1d(i) = k(src.at1d(i));
        }
    }
    src.copy(dst);
}

int main(int argc, char* argv[]) {

    // -- determine page number --
    char s1[1024];
    strcpy(s1, argv[2]);
    strg dd = basename(s1);
    int pageNo = atoi(dd.erase(4).c_str());

    // -- create output directory --
    strcpy(s1, argv[2]);
    strg outDir = dirname(s1);
    sprintf_append(outDir, "/%04d", pageNo);
    mkdir(outDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    // -- read original page --
    bytearray pageOrig;
    read_image_gray(pageOrig, argv[1]);

    // -- binarize page --
    bytearray pageBin;
    autodel<IBinarize> binarizer(make_BinarizeBySauvola());
    binarizer->binarize(pageBin, pageOrig);

    // -- page segmentation --
    intarray pageSeg;
    autodel<ISegmentPage> segmenter(make_SegmentPageByRAST());
    segmenter->segment(pageSeg, pageBin);

    // -- tight numbering --
    renumber(pageSeg);
    printf("%d\n", max(pageSeg));

    // -- bounding boxes of lines --
    rectarray linesBBoxes;
    bounding_boxes(linesBBoxes,pageSeg);

    // -- create lines data structure --
    objlist<Line> lines;
    for(int i=1; i<linesBBoxes.length(); i++) {
        lines.push().bbox = linesBBoxes(i);
    }
    linesBBoxes.clear();

    ustrg space;
    space.push_back(' ');

    // -- reading characters with bounding boxes from text file --
    FILE* file = fopen(argv[2], "r");
    while(!feof(file)) {
        // -- read one line into variables --
        rectangle box;
        char c[5];
        ustrg unicode;
        if(fscanf(file, "%d,%d,%d,%d,%[^\n]\n", &box.x0, &box.y1, &box.x1, &box.y0, c) == 5) {
            box.y0 = pageBin.dim(1) - box.y0;
            box.y1 = pageBin.dim(1) - box.y1;
            unicode.clear();
            unicode.utf8Decode(c, strlen(c));
            // -- find according line --
            int line = -1;
            double maxCover = 0.0;
            for(int i=0; i<lines.length(); i++) {
                double cover = box.fraction_covered_by(lines(i).bbox);
                if(cover > maxCover) {
                    line = i;
                    maxCover = cover;
                }
            }
            if(maxCover < 0.3) {
                fprintf(stderr, "bad line match: %0.2f (%s)\n", maxCover, c);
            } else if(unicode != space) {
                // -- add character to line --
                lines(line).charBBoxes.push(box);
                lines(line).trans.append(unicode);
            } else {
                printf("skip space\n");
            }
        }
    }
    fclose(file);
    
    // -- over segment all lines and match character bounding boxes --
    autodel<ISegmentLine> lineSegmenter(make_CurvedCutWithCcSegmenter());
    lineSegmenter->set("debug", "debug.png");
    lineSegmenter->set("min_thresh", 300);
    for(int i=0; i<lines.length(); i++) {
        Line& line = lines(i);
        if(line.trans.length() == 0) {
            continue;
        }
        bytearray lineBin;
        extract_subimage(lineBin, pageBin, line.bbox.x0, line.bbox.y0, line.bbox.x1, line.bbox.y1);
        bytearray lineOrig;
        extract_subimage(lineOrig, pageOrig, line.bbox.x0, line.bbox.y0, line.bbox.x1, line.bbox.y1);
        int p = 10;
        pad_by(lineBin, p, p, (unsigned char)255);
        // -- offset character bounding boxes --
        for(int j=0; j<line.charBBoxes.length(); j++) {
            line.charBBoxes(j).x0 -= line.bbox.x0 - p;
            line.charBBoxes(j).x1 -= line.bbox.x0 - p;
            line.charBBoxes(j).y0 -= line.bbox.y0 - p;
            line.charBBoxes(j).y1 -= line.bbox.y0 - p;
        }
        intarray lineOverSeg;
        lineSegmenter->charseg(lineOverSeg, lineBin);
        intarray lineSeg;
        ocr_bboxes_to_charseg(lineSeg, line.charBBoxes, lineOverSeg);

        strg outFile;

        intarray lineSegSave;
        lineSegSave.copy(lineSeg);
        replace_values(lineSegSave, 0, 0xFFFFFF);

        sprintf(outFile, "%s/%04d.png", outDir.c_str(), i);
        write_image_gray(outFile, lineOrig);

        sprintf(outFile, "%s/%04d.bin.png", outDir.c_str(), i);
        write_image_gray(outFile, lineBin);

        sprintf(outFile, "%s/%04d.cseg.gt.png", outDir.c_str(), i);
        write_image_packed(outFile, lineSegSave);

        simple_recolor(lineSeg);
        sprintf(outFile, "%s/%04d.cseg.rec.png", outDir.c_str(), i);
        write_image_packed(outFile, lineSeg);

        sprintf(outFile, "%s/%04d.gt.txt", outDir.c_str(), i);
        //char utf8Line[line.trans.length()*4+1];
        FILE* file = fopen(outFile, "w");
        fputsUTF8(line.trans, file);
        fclose(file);
    }


    return 0;
}
