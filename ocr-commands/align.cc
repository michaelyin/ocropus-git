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
#include "ocr-commands.h"

namespace ocropus {
    int main_align(int argc,char **argv) {
        param_bool abort_on_error("abort_on_error",0,"abort recognition if there is an unexpected error");
        param_string suffix("suffix",0,"suffix for writing the ground truth (e.g., 'gt')");
        param_string cbookstore("bookstore","SmartBookStore","storage abstraction for book");
        param_string gt_type("gt_type","transcript","kind of ground truth: transcript, fst, pagefst");
        param_int beam_width("beam_width", 100, "number of nodes in a beam generation");
        autodel<IBookStore> bookstore;
        make_component(bookstore,cbookstore);
        bookstore->setPrefix(argv[1]);
//#pragma omp parallel for
        for(int page=0;page<bookstore->numberOfPages();page++) {
            int nlines = bookstore->linesOnPage(page);
//#pragma omp parallel for
            for(int j=0;j<nlines;j++) {
                int line = bookstore->getLineId(page,j);
                debugf("progress","align %04d %06x\n",page,line);
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
                strg path = bookstore->path(page,line,0,"fst");
                try {
                    if(!file_exists(path)) {
                        debugf("warn","%s: not found\n",path.c_str());
                        continue;
                    }
                    fst->load(path);
                } catch(const char *error) {
                    fprintf(stderr,"%s: %s\n",path.c_str(),error);
                    if(abort_on_error) abort();
                } catch(...) {
                    fprintf(stderr,"%s: cannot load\n",path.c_str());
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
                    strg s;
                    s = bookstore->path(page,line,suffix,"txt");
                    fprintf(stdio(s,"w"),"%s\n",utf8Output.c_str());
                } catch(const char *err) {
                    fprintf(stderr,"ERROR in transcript output: %s\n",err);
                    if(abort_on_error) abort();
                }
            }
        }
        return 0;
    }
}
