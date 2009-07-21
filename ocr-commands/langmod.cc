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
    extern param_int beam_width;
    extern param_int abort_on_error;
    extern param_string lmodel;
    extern void nustring_convert(iucstring &output,nustring &str);
    extern void nustring_convert(nustring &output,iucstring &str);

    static void store_costs(const char *base, floatarray &costs) {
        iucstring s;
        s = base;
        s.append(".costs");
        stdio stream(s,"w");
        for(int i=0;i<costs.length();i++) {
            fprintf(stream,"%d %g\n",i,costs(i));
        }
    }

    static void rseg_to_cseg(intarray &cseg, intarray &rseg, intarray &ids) {
        intarray map(max(rseg) + 1);
        map.fill(0);
        int color = 0;
        for(int i = 0; i < ids.length(); i++) {
            if(!ids[i]) continue;
            color++;
            int start = ids[i] >> 16;
            int end = ids[i] & 0xFFFF;
            if(start > end)
                throw "segmentation encoded in IDs looks seriously broken!\n";
            if(start >= map.length() || end >= map.length())
                throw "segmentation encoded in IDs doesn't fit!\n";
            for(int j = start; j <= end; j++)
                map[j] = color;
        }
        cseg.makelike(rseg);
        for(int i = 0; i < cseg.length1d(); i++)
            cseg.at1d(i) = map[rseg.at1d(i)];
    }

    // Read a line and make an FST out of it.
    void read_transcript(IGenericFst &fst, const char *path) {
        nustring gt;
        fgetsUTF8(gt, stdio(path, "r"));
        fst_line(fst, gt);
    }

    // Reads a "ground truth" FST (with extra spaces) by basename
    void read_gt(IGenericFst &fst, const char *base) {
        strbuf gt_path;
        gt_path = base;
        gt_path += ".gt.txt";

        read_transcript(fst, gt_path);
        for(int i = 0; i < fst.nStates(); i++)
            fst.addTransition(i, i, 0, 0, ' ');
    }

    int main_align(int argc,char **argv) {
        throw "FIXME: main_align TEMPORARILY DISABLED";
#if 0
        if(argc!=2) throw "usage: ... dir";
        iucstring s;
        s = argv[1];
        s += "/[0-9][0-9][0-9][0-9]/[0-9][0-9][0-9][0-9].fst";
        Glob files(s);
        for(int index=0;index<files.length();index++) {
            if(index%1000==0)
                debugf("info","%s (%d/%d)\n",files(index),index,files.length());

            iucstring base;
            base = files(index);
            base.erase(base.length()-4);

            autodel<OcroFST> gt_fst(make_OcroFST());
            read_gt(*gt_fst, base);

            autodel<OcroFST> fst(make_OcroFST());
            fst->load(files(index));
            nustring str;
            intarray v1;
            intarray v2;
            intarray in;
            intarray out;
            floatarray costs;
            try {
                beam_search(v1, v2, in, out, costs,
                            *fst, *gt_fst, beam_width);
                // recolor rseg to cseg
            } catch(const char *error) {
                fprintf(stderr,"ERROR in bestpath: %s\n",error);
                if(abort_on_error) abort();
            }
            try {
                rseg_to_cseg(base, in);
                store_costs(base, costs);
                debugf("dcost","--------------------------------\n");
                for(int i=0;i<out.length();i++) {
                    debugf("dcost","%3d %10g %c\n",i,costs(i),out(i));
                }
            } catch(const char *err) {
                fprintf(stderr,"ERROR in cseg reconstruction: %s\n",err);
                if(abort_on_error) abort();
            }
        }
        return 0;
#endif
    }


    int main_fsts2text(int argc,char **argv) {
        if(argc!=2) throw "usage: lmodel=... ocropus fsts2text dir";
        autodel<OcroFST> langmod(make_OcroFST());
        try {
            langmod->load(lmodel);
        } catch(const char *s) {
            throwf("%s: failed to load (%s)",(const char*)lmodel,s);
        } catch(...) {
            throwf("%s: failed to load language model",(const char*)lmodel);
        }

        autodel<IBookStore> bookstore;
        extern param_string cbookstore;
        make_component(bookstore,cbookstore);
        bookstore->setPrefix(argv[1]);
//#pragma omp parallel for schedule(dynamic,20)
        for(int page=0;page<bookstore->numberOfPages();page++) {
            int nlines = bookstore->linesOnPage(page);
//#pragma omp parallel for private(linerec) shared(finished) schedule(dynamic,4)
                for(int j=0;j<nlines;j++) {
                    int line = bookstore->getLineId(page,j);
                    debugf("progress","page %04d %06x\n",page,line);
                    autodel<OcroFST> fst(make_OcroFST());
                    fst->load(bookstore->path(page,line,0,"fst"));
                    nustring str;
                    try {
                        intarray v1;
                        intarray v2;
                        intarray in;
                        intarray out;
                        floatarray costs;
                        beam_search(v1, v2, in, out, costs,
                                    *fst, *langmod, beam_width);
                        double cost = sum(costs);
                        remove_epsilons(str, out);
                        if(cost < 1e10) {
                            utf8strg utf8Output;
                            str.utf8EncodeTerm(utf8Output);
                            debugf("transcript","%04d %06x\t%s\n",page,line, utf8Output.c_str());
                            try {
                                intarray rseg;
                                read_image_packed(rseg, bookstore->path(page,line,"rseg","png"));
                                make_line_segmentation_black(rseg);
                                intarray cseg;
                                rseg_to_cseg(cseg, rseg, in);
                                ::make_line_segmentation_white(cseg);
                                write_image_packed(bookstore->path(page,line,"cseg","png"),cseg);
                            } catch(const char *err) {
                                fprintf(stderr,"ERROR in cseg reconstruction: %s\n",err);
                                if(abort_on_error) abort();
                            }
                            iucstring s(bookstore->path(page,line,0,"txt"));
                            fprintf(stdio(s,"w"),"%s\n",utf8Output.c_str());
                        } else {
                            debugf("info","%04d %06x failed to match language model\n",page,line);
                        }
                    } catch(const char *error) {
                        fprintf(stderr,"ERROR in bestpath: %s\n",error);
                        if(abort_on_error) abort();
                    }
                }
        }

        return 0;
    }

    int main_fsts2bestpaths(int argc,char **argv) {
        if(argc!=2) throw "usage: ... dir";
        autodel<IBookStore> bookstore;
        extern param_string cbookstore;
        make_component(bookstore,cbookstore);
        bookstore->setPrefix(argv[1]);
//#pragma omp parallel for schedule(dynamic,20)
        for(int page=0;page<bookstore->numberOfPages();page++) {
            int nlines = bookstore->linesOnPage(page);
//#pragma omp parallel for private(linerec) shared(finished) schedule(dynamic,4)
            for(int j=0;j<nlines;j++) {
                int line = bookstore->getLineId(page,j);
                debugf("progress","page %04d %06x\n",page,line);
                autodel<OcroFST> fst(make_OcroFST());
                fst->load(bookstore->path(page,line,0,"fst"));
                nustring str;
                try {
                    fst->bestpath(str);
                    utf8strg utf8Output;
                    str.utf8EncodeTerm(utf8Output);
                    debugf("transcript","%04d %06x\t%s\n",page,line,utf8Output.c_str());
                    iucstring s(bookstore->path(page,line,0,"txt"));
                    fprintf(stdio(s,"w"),"%s",utf8Output.c_str());
                } catch(const char *error) {
                    fprintf(stderr,"ERROR in bestpath: %s\n",error);
                    if(abort_on_error) abort();
                }
            }
        }
        return 0;
    }
}
