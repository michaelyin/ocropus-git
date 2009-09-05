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

#ifndef DEFAULT_DATA_DIR
#define DEFAULT_DATA_DIR "/usr/local/share/ocropus/models/"
#endif

namespace ocropus {
    extern param_int beam_width;
    extern param_bool abort_on_error;
    extern void ustrg_convert(strg &output,ustrg &str);
    extern void ustrg_convert(ustrg &output,strg &str);

    static void store_costs(const char *base, floatarray &costs) {
        stdio stream(base,"w");
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
        ustrg gt;
        fgetsUTF8(gt, stdio(path, "r"));
        fst_line(fst, gt);
    }

    // Reads a "ground truth" FST (with extra spaces) by basename
    void read_gt(IGenericFst &fst, const char *base) {
        strbuf gt_path;
        gt_path = base;
        gt_path += "gt.txt";

        read_transcript(fst, gt_path);
        for(int i = 0; i < fst.nStates(); i++)
            fst.addTransition(i, i, 0, 0, ' ');
    }

    int main_align(int argc,char **argv) {
        param_string cbookstore("bookstore","SmartBookStore","storage abstraction for book");
        param_string gt_type("gt_type","transcript","kind of ground truth: transcript, fst, pagefst");
        autodel<IBookStore> bookstore;
        make_component(bookstore,cbookstore);
        bookstore->setPrefix(argv[1]);
//#pragma omp parallel for
        for(int page=0;page<bookstore->numberOfPages();page++) {
            int nlines = bookstore->linesOnPage(page);
//#pragma omp parallel for
            for(int j=0;j<nlines;j++) {
                int line = bookstore->getLineId(page,j);
                debugf("progress","page %04d %06x\n",page,line);
                autodel<OcroFST> gt_fst(make_OcroFST());
                if(!strcmp(gt_type,"transcript")) {
                    read_gt(*gt_fst, bookstore->path(page,line,0,""));
                } else if(!strcmp(gt_type,"fst")) {
                    throw "unimplemented";
                } else if(!strcmp(gt_type,"pagefst")) {
                    gt_fst->load(bookstore->path(page,-1,"gt","fst"));
                } else {
                    throw "unknown gt_type";
                }

                autodel<OcroFST> fst(make_OcroFST());
                try {
                    if(!file_exists(bookstore->path(page,line,0,"fst"))) {
                        debugf("warn","%s: not found, needed for alignment",
                               bookstore->path(page,line,0,"fst"));
                        continue;
                    }
                    fst->load(bookstore->path(page,line,0,"fst"));
                } catch(const char *error) {
                    fprintf(stderr,"ERROR loading fst: %s\n",error);
                    if(abort_on_error) abort();
                } catch(...) {
                    strg s = bookstore->path(page,line,0,"fst");
                    fprintf(stderr,"ERROR loading fst: %s\n",s.c_str());
                    if(abort_on_error) abort();
                }
                ustrg str;
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
                double cost = sum(costs);
                try {
                    intarray rseg;
                    read_image_packed(rseg, bookstore->path(page,line,"rseg","png"));
                    make_line_segmentation_black(rseg);
                    intarray cseg;
                    rseg_to_cseg(cseg, rseg, in);
                    ::make_line_segmentation_white(cseg);
                    write_image_packed(bookstore->path(page,line,"cseg","png"),cseg);
                    store_costs(bookstore->path(page,line,0,"costs"), costs);
                    debugf("dcost","--------------------------------\n");
                    for(int i=0;i<out.length();i++) {
                        debugf("dcost","%3d %10g %c\n",i,costs(i),out(i));
                    }
                } catch(const char *err) {
                    fprintf(stderr,"ERROR in cseg reconstruction: %s\n",err);
                    if(abort_on_error) abort();
                }
                try {
                    ustrg str;
                    remove_epsilons(str, out);
                    utf8strg utf8Output;
                    str.utf8EncodeTerm(utf8Output);
                    debugf("transcript","%04d %06x\t%g\t%s\n",page,line,cost,utf8Output.c_str());
                    strg s(bookstore->path(page,line,0,"txt"));
                    fprintf(stdio(s,"w"),"%s\n",utf8Output.c_str());
                } catch(const char *err) {
                    fprintf(stderr,"ERROR in transcript output: %s\n",err);
                    if(abort_on_error) abort();
                }
            }
        }
        return 0;
    }

    void scale_fst(OcroFST &fst,float scale) {
        if(fabs(scale-1.0)<1e-6) return;
        for(int i=0;i<fst.nStates();i++) {
            fst.costs(i) *= scale;
            float accept = fst.acceptCost(i);
            if(accept>=0 && accept<1e37)
                fst.setAcceptCost(i,accept*scale);
        }
    }



    int main_fsts2text(int argc,char **argv) {
        param_float langmod_scale("langmod_scale",1.0,"scale factor for language model");
        param_string lmodel("lmodel",DEFAULT_DATA_DIR "default.fst","language model used for recognition");
        param_string cbookstore("bookstore","SmartBookStore","storage abstraction for book");
        if(argc!=2) throw "usage: lmodel=... ocropus fsts2text dir";
        autodel<OcroFST> langmod;
        try {
            langmod = make_OcroFST();
            langmod->load(lmodel);
            langmod = 0;
        } catch(const char *s) {
            throwf("%s: failed to load (%s)",(const char*)lmodel,s);
        } catch(...) {
            throwf("%s: failed to load language model",(const char*)lmodel);
        }

        autodel<IBookStore> bookstore;
        make_component(bookstore,cbookstore);
        bookstore->setPrefix(argv[1]);
        debugf("info","langmod_scale = %g\n",float(langmod_scale));
        for(int page=0;page<bookstore->numberOfPages();page++) {
            int nlines = bookstore->linesOnPage(page);
#pragma omp parallel for private(langmod)
                for(int j=0;j<nlines;j++) {
                    if(!langmod) {
                        langmod = make_OcroFST();
                        langmod->load(lmodel);
                        scale_fst(*langmod,langmod_scale);
                        CHECK(!!langmod);
                    }
                    int line = bookstore->getLineId(page,j);
                    debugf("progress","page %04d %06x\n",page,line);
                    autodel<OcroFST> fst(make_OcroFST());
                    try {
                        fst->load(bookstore->path(page,line,0,"fst"));
                        CHECK(!!fst);
                    } catch(const char *error) {
                        fprintf(stderr,"%04d %06x: can't load fst: %s\n",page,line,error);
                        if(abort_on_error) abort();
                        continue;
                    }
                    ustrg str;
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
                            strg s(bookstore->path(page,line,0,"txt"));
                            fprintf(stdio(s,"w"),"%s\n",utf8Output.c_str());
                        } else {
                            debugf("warn","%04d %06x failed to match language model\n",page,line);
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
        param_string cbookstore("bookstore","SmartBookStore","storage abstraction for book");
        if(argc!=2) throw "usage: ... dir";
        autodel<IBookStore> bookstore;
        make_component(bookstore,cbookstore);
        bookstore->setPrefix(argv[1]);
        for(int page=0;page<bookstore->numberOfPages();page++) {
            int nlines = bookstore->linesOnPage(page);
#pragma omp parallel for
            for(int j=0;j<nlines;j++) {
                int line = bookstore->getLineId(page,j);
                debugf("progress","page %04d %06x\n",page,line);
                autodel<OcroFST> fst(make_OcroFST());
                try {
                    fst->load(bookstore->path(page,line,0,"fst"));
                } catch(const char *error) {
                    fprintf(stderr,"cannot load %04d %06x: %s\n",page,line,error);
                    if(abort_on_error) abort();
                    continue;
                }
                ustrg str;
                try {
                    fst->bestpath(str);
                    utf8strg utf8Output;
                    str.utf8EncodeTerm(utf8Output);
                    debugf("transcript","%04d %06x\t%s\n",page,line,utf8Output.c_str());
                    strg s(bookstore->path(page,line,0,"txt"));
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
