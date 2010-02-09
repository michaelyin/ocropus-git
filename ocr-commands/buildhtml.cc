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

    void hocr_dump_line(FILE *output,IBookStore &bookstore,
                        RegionExtractor &r, int page, int line, int h) {
        fprintf(output, "<span class=\"ocr_line\"");
        if(line > 0 && line < r.length()) {
            fprintf(output, " title=\"bbox %d %d %d %d\"",
                        r.x0(line), h - 1 - r.y0(line),
                        r.x1(line), h - 1 - r.y1(line));
        }
        fprintf(output, ">\n");
        ustrg s;
        if (file_exists(bookstore.path(page,line,0,"txt"))) {
            fgetsUTF8(s, stdio(bookstore.path(page,line,0,"txt"), "r"));
            fwriteUTF8(s, output);
        }
        fprintf(output, "</span>");
    }

    void hocr_dump_page(FILE *output, IBookStore & bookstore, int page) {
        strg pattern;

        intarray page_seg;
        //bookstore.getPageSegmentation(page_seg, page);
        if (!bookstore.getPage(page_seg, page, "pseg")) {
            if(page>0)
                debugf("warn","%d: page not found\n",page);
                return;
        }
        int h = page_seg.dim(1);

        RegionExtractor regions;
        regions.setPageLines(page_seg);
        rectarray bboxes;

        fprintf(output, "<div class=\"ocr_page\">\n");
        
        int nlines = bookstore.linesOnPage(page);

        for(int i=0;i<nlines;i++) {
            int line = bookstore.getLineId(page,i);
            hocr_dump_line(output, bookstore, regions, page, line, h);
        }
        fprintf(output, "</div>\n");
    }

    int main_buildhtml(int argc,char **argv) {
        param_string cbookstore("bookstore","SmartBookStore","storage abstraction for book");
        if(argc!=2) throw "usage: ... dir";
        autodel<IBookStore> bookstore;
        make_component(bookstore,cbookstore);
        bookstore->setPrefix(argv[1]);
        
        FILE *output = stdout;
        hocr_dump_preamble(output);
        fprintf(output, "<html>\n");
        hocr_dump_head(output);
        fprintf(output, "<body>\n");
        int npages = bookstore->numberOfPages();
        for(int page=0;page<npages;page++) {
            hocr_dump_page(output, *bookstore, page);
        }
        fprintf(output, "</body>\n");
        fprintf(output, "</html>\n");
        return 0;
    }

    int main_cleanhtml(int argc,char **argv) {
        throw Unimplemented();
    }
}
