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
// Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org

#define __warn_unused_result__ __far__

#include <cctype>
#include <sys/types.h>
#include <sys/stat.h>
#include <glob.h>
#include <unistd.h>
#include "colib/colib.h"
#include "iulib/iulib.h"
#include "ocropus.h"
#include "glinerec.h"
#include "bookstore.h"

namespace ocropus {
    void hocr_dump_preamble(FILE *output) {
        fprintf(output, "<!DOCTYPE html\n");
        fprintf(output, "   PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\n");
        fprintf(output, "   http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n");
    }

    void hocr_dump_head(FILE *output) {
        fprintf(output, "<head>\n");
        fprintf(output, "<meta name=\"ocr-capabilities\" content=\"ocr_line ocr_page\" />\n");
        fprintf(output, "<meta name=\"ocr-langs\" content=\"en\" />\n");
        fprintf(output, "<meta name=\"ocr-scripts\" content=\"Latn\" />\n");
        fprintf(output, "<meta name=\"ocr-microformats\" content=\"\" />\n");
        fprintf(output, "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />");
        fprintf(output, "<title>OCR Output</title>\n");
        fprintf(output, "</head>\n");
    }

    void hocr_dump_line(FILE *output, const char *path,
                        RegionExtractor &r, int index, int h) {
        fprintf(output, "<span class=\"ocr_line\"");
        if(index > 0 && index < r.length()) {
            fprintf(output, " title=\"bbox %d %d %d %d\"",
                        r.x0(index), h - 1 - r.y0(index),
                        r.x1(index), h - 1 - r.y1(index));
        }
        fprintf(output, ">\n");
        ustrg s;
        fgetsUTF8(s, stdio(path, "r"));
        fwriteUTF8(s, output);
        fprintf(output, "</span>");
    }

    void hocr_dump_page(FILE *output, const char *path) {
        strg pattern;

        sprintf(pattern,"%s.pseg.png",path);
        if(!file_exists(pattern))
            sprintf(pattern,"%s.seg.png",path); // temporary backwards compatibility
        intarray page_seg;
        read_image_packed(page_seg, pattern);
        int h = page_seg.dim(1);

        RegionExtractor regions;
        regions.setPageLines(page_seg);
        rectarray bboxes;

        sprintf(pattern,"%s/[0-9][0-9][0-9][0-9].txt",path);
        // FIXME pathname dependency; replace with IBookStore object
        Glob lines(pattern);
        fprintf(output, "<div class=\"ocr_page\">\n");
        for(int i=0;i<lines.length();i++) {
            // we have to figure out line number from the path because
            // the loop index is unreliable: it skips lines that didn't work
            pattern = lines(i);
            pattern.erase(pattern.length() - 4); // cut .txt
            int line_index = atoi(pattern.substr(pattern.length() - 4));
            hocr_dump_line(output, lines(i), regions, line_index, h);
        }
        fprintf(output, "</div>\n");
    }

    int main_buildhtml(int argc,char **argv) {
        if(argc!=2) throw "usage: ... dir";
        strg pattern;
        sprintf(pattern,"%s/[0-9][0-9][0-9][0-9]",argv[1]);
        Glob pages(pattern);
        FILE *output = stdout;
        hocr_dump_preamble(output);
        fprintf(output, "<html>\n");
        hocr_dump_head(output);
        fprintf(output, "<body>\n");
        for(int i = 0; i < pages.length(); i++) {
            hocr_dump_page(output, pages(i));
        }
        fprintf(output, "</body>\n");
        fprintf(output, "</html>\n");
        return 0;
    }

    int main_cleanhtml(int argc,char **argv) {
        throw Unimplemented();
    }
}
