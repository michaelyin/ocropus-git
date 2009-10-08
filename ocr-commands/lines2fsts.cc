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
#include "linesegs.h"
#include "ocr-commands.h"

namespace ocropus {

    using namespace iulib;
    using namespace colib;
    using namespace ocropus;
    using namespace narray_ops;
    using namespace glinerec;

    int main_lines2fsts(int argc,char **argv) {
        param_bool abort_on_error("abort_on_error",0,"abort recognition if there is an unexpected error");
        param_string cbookstore("bookstore","SmartBookStore","storage abstraction for book");
        param_string cmodel("cmodel",DEFAULT_DATA_DIR "default.model","character model used for recognition");
        param_bool save_fsts("save_fsts",1,"save the fsts (set to 0 for eval-only in lines2fsts)");
        param_bool continue_partial("continue_partial",0,"don't compute outputs that already exist");
        param_float maxheight("max_line_height",300,"maximum line height");
        param_float maxaspect("max_line_aspect",1.0,"maximum line aspect ratio");
        if(argc!=2) throw "usage: cmodel=... ocropus lines2fsts dir";
        dinit(512,512);
        autodel<IRecognizeLine> linerec;
        autodel<IBookStore> bookstore;
        make_component(bookstore,cbookstore);
        bookstore->setPrefix(argv[1]);
        int finished = 0;
        int nfiles = 0;
        for(int page=0;page<bookstore->numberOfPages();page++)
            nfiles += bookstore->linesOnPage(page);
        int eval_total=0,eval_tchars=0,eval_pchars=0,eval_lines=0,eval_no_ground_truth=0;
        debugf("info","cmodel=%s\n",(const char *)cmodel);
        for(int page=0;page<bookstore->numberOfPages();page++) {
            int nlines = bookstore->linesOnPage(page);
#pragma omp parallel for private(linerec) shared(finished) schedule(dynamic,4)
            for(int j=0;j<nlines;j++) {
                int line = bookstore->getLineId(page,j);
                try {
#pragma omp critical
                    try {
                        if(!linerec) linerec_load(linerec,cmodel);
                    } catch(...) {
                        debugf("info","loading %s failed\n",(const char *)cmodel);
                        abort(); // can't do much else in OpenMP
                    }
                    debugf("progress","page %04d line %06x\n",page,line);
                    if(continue_partial) {
                        strg s;
                        FILE *stream = bookstore->open("r",page,line,0,"fst");
                        if(stream) {
                            fclose(stream);
                            // finished++;
                            // eval_lines++;
                            continue;
                        }
                    }
                    strg line_path_ = bookstore->path(page,line);
                    const char *line_path = (const char *)line_path_;
                    debugf("linepath","%s\n",line_path);
                    bytearray image;
                    // FIXME output binary versions, intermediate results for debugging
#pragma omp critical
                    bookstore->getLine(image,page,line);
                    autodel<IGenericFst> result(make_OcroFST());
                    intarray segmentation;
                    try {
                        CHECK_ARG(image.dim(1)<maxheight);
                        CHECK_ARG(image.dim(1)*1.0/image.dim(0)<maxaspect);
                        try {
                            linerec->recognizeLine(segmentation,*result,image);
                        } catch(Unimplemented unimplemented) {
                            linerec->recognizeLine(*result,image);
                        }
                    } catch(BadTextLine &error) {
                        debugf("warn","skipping %s (bad text line)\n",line_path);
                        continue;
                    } catch(const char *error) {
                        debugf("warn","skipping %s (%s)\n",line_path,error);
                        if(abort_on_error) abort();
                        continue;
                    } catch(...) {
                        debugf("warn","skipping %s (unknown exception)\n",line_path);
                        if(abort_on_error) abort();
                        continue;
                    }

                    if(save_fsts) {
                        strg s;
                        s = bookstore->path(page,line,0,"fst");
                        result->save(s);
                        if(segmentation.length()>0) {
                            dsection("line_segmentation");
                            make_line_segmentation_white(segmentation);
                            s = bookstore->path(page,line,"rseg","png");
                            write_image_packed(s,segmentation);
                            dshowr(segmentation);
                            dwait();
                        }
                    }

                    ustrg predicted;
                    try {
                        result->bestpath(predicted);
                        utf8strg utf8Predicted;
                        predicted.utf8EncodeTerm(utf8Predicted);
                        debugf("transcript","%04d %06x\t%s\n",page,line,utf8Predicted.c_str());
#pragma omp critical
                        if(save_fsts) bookstore->putLine(predicted,page,line);
                    } catch(const char *error) {
                        debugf("warn","%s in bestpath\n",error);
                        if(abort_on_error) abort();
                        continue;
                    } catch(...) {
                        debugf("warn","error in bestpath\n",error);
                        if(abort_on_error) abort();
                        continue;
                    }

                    ustrg truth;
#pragma omp critical
                    if(bookstore->getLine(truth,page,line,"gt")) try {
                            // FIXME not unicode clean
                            cleanup_for_eval(truth);
                            cleanup_for_eval(predicted);
                            debugf("truth","%04d %04d\t%s\n",page,line,truth.c_str());
                            float dist = edit_distance(truth,predicted);
#pragma omp atomic
                            eval_total += dist;
#pragma omp atomic
                            eval_tchars += truth.length();
#pragma omp atomic
                            eval_pchars += predicted.length();
#pragma omp atomic
                            eval_lines++;
                        } catch(...) {
#pragma omp atomic
                            eval_no_ground_truth++;
                        }

#pragma omp critical (finished_counter)
                    {
                        finished++;
                        if(finished%100==0) {
                            if(eval_total>0)
                                debugf("info","finished %d/%d estimate %g errs %d ntrue %d npred %d lines %d nogt %d\n",
                                       finished,nfiles,
                                       eval_total/float(eval_tchars),eval_total,eval_tchars,eval_pchars,
                                       eval_lines,eval_no_ground_truth);
                            else
                                debugf("info","finished %d/%d\n",finished,nfiles);
                        }
                    }
                } catch(const char *error) {
                    debugf("error","%s in recognizeLine\n",error);
                    if(abort_on_error) abort();
                    continue;
                } catch(...) {
                    debugf("error","error in recognizeLine\n");
                    if(abort_on_error) abort();
                    continue;
                }
            }
        }

        debugf("info","rate %g errs %d ntrue %d npred %d lines %d nogt %d\n",
               eval_total/float(eval_tchars),eval_total,eval_tchars,eval_pchars,
               eval_lines,eval_no_ground_truth);
        return 0;
    }

}
