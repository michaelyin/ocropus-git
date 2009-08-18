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

#define DLOPEN

#ifdef DLOPEN
#include <dlfcn.h>
#endif

namespace glinerec {
    IRecognizeLine *make_Linerec();
    const char *command = "???";
}

namespace ocropus {
    using namespace iulib;
    using namespace colib;
    using namespace ocropus;
    using namespace narray_ops;
    using namespace glinerec;

    param_bool abort_on_error("abort_on_error",0,"abort recognition if there is an unexpected error");
    param_bool save_fsts("save_fsts",1,"save the fsts (set to 0 for eval-only in lines2fsts)");
    param_bool retrain("retrain",0,"perform retraining");
    param_bool retrain_threshold("retrain_threshold",100,"only retrain on characters with a cost lower than this");
    param_int nrecognize("nrecognize",1000000,"maximum number of lines to predict (for quick testing)");
    param_int ntrain("ntrain",10000000,"max number of training examples");
    param_bool continue_partial("continue_partial",0,"don't compute outputs that already exist");
    param_bool old_csegs("old_csegs",0,"use old csegs (spaces are not counted)");
    param_float maxheight("max_line_height",300,"maximum line height");
    param_float maxaspect("max_line_aspect",1.0,"maximum line aspect ratio");

    param_string cbookstore("bookstore","SmartBookStore","storage abstraction for book");
    param_string cbinarizer("binarizer","","binarization component to use");
    param_string csegmenter("psegmenter","SegmentPageByRAST","segmenter to use at the page level");
#ifdef DLOPEN
    param_string extension("extension",0,"preload extensions");
#endif

#define DEFAULT_DATA_DIR "/usr/local/share/ocropus/models/"

    param_string cmodel("cmodel",DEFAULT_DATA_DIR "default.model","character model used for recognition");
    param_string lmodel("lmodel",DEFAULT_DATA_DIR "default.fst","language model used for recognition");


    param_string eval_flags("eval_flags","space","which features to ignore during evaluation");

    void cleanup_for_eval(strg &s) {
        bool space = strflag(eval_flags,"space");
        bool scase = strflag(eval_flags,"case");
        bool nonanum = strflag(eval_flags,"nonanum");
        bool nonalpha = strflag(eval_flags,"nonalpha");
        strg result = "";
        for(int i=0;i<s.length();i++) {
            int c = s[i];
            if(space && c==' ') continue;
            if(nonanum && !isalnum(c)) continue;
            if(nonalpha && !isalpha(c)) continue;
            if(c<32||c>127) continue;
            if(scase && isupper(c)) c = tolower(c);
            result.push_back(c);
        }
        s = result;
    }

    void cleanup_for_eval(ustrg &s) {
        bool space = strflag(eval_flags,"space");
        bool scase = strflag(eval_flags,"case");
        bool nonanum = strflag(eval_flags,"nonanum");
        bool nonalpha = strflag(eval_flags,"nonalpha");
        ustrg result;
        for(int i=0;i<s.length();i++) {
            int c = s[i].ord();
            if(space && c==' ') continue;
            if(nonanum && !isalnum(c)) continue;
            if(nonalpha && !isalpha(c)) continue;
            if(c<32||c>127) continue;
            if(scase && isupper(c)) c = tolower(c);
            result.push_back(nuchar(c));
        }
        s = result;
    }

    // these are used for the single page recognizer
    param_int beam_width("beam_width", 100, "number of nodes in a beam generation");

    void chomp_extension(char *s) {
        char *p = s+strlen(s);
        while(p>s) {
            --p;
            if(*p=='/') break;
            if(*p=='.') *p = 0;
        }
    }

    void linerec_load(autodel<IRecognizeLine> &linerec,const char *cmodel) {
        linerec = glinerec::make_Linerec();
        try {
            load_component(stdio(cmodel,"r"),linerec);
        } catch(...) {
            debugf("debug","%s: failed to load as component, trying as object\n",cmodel);
            try {
                linerec->load(cmodel);
            } catch(const char *s) {
                throwf("%s: failed to load (%s)",(const char*)cmodel,s);
            } catch(...) {
                throwf("%s: failed to load character model",(const char*)cmodel);
            }
        }
    }

    int main_threshold(int argc,char **argv) {
        CHECK(argc==3);
        param_string name("binarizer","BinarizeBySauvola","binarization method");
        autodel<IBinarize> binarizer;
        make_component(binarizer,name);
        debugf("info","using %s\n",binarizer->name());
        bytearray image;
        read_image_gray(image,argv[1]);
        bytearray bin;
        binarizer->binarize(bin,image);
        write_image_gray(argv[2],bin);
        return 0;
    }

    int main_book2pages(int argc,char **argv) {
        int pageno = 0;
        const char *outdir = argv[1];

        autodel<IBookStore> bookstore;
        make_component(bookstore,cbookstore);

        if(mkdir(outdir,0777)) {
            fprintf(stderr,"error creating OCR working directory\n");
            perror(outdir);
            exit(1);
        }

        bookstore->setPrefix(outdir);

        strg s;
        for(int arg=2;arg<argc;arg++) {
            Pages pages;
            pages.parseSpec(argv[arg]);
            while(pages.nextPage()) {
                pageno++;
                debugf("info","page %d\n",pageno);
                mkdir(s,0777);
                bytearray page_binary,page_gray;
                pages.getGray(page_gray);
                bookstore->putPage(page_gray,pageno);
                pages.getBinary(page_binary);
                bookstore->putPage(page_binary,pageno,"bin");
            }
        }
        return 0;
    }


    int main_lines2fsts(int argc,char **argv) {
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
                        stdio stream;
#pragma omp critical
                        stream = bookstore->open("r",page,line,0,"fst");
                        if(stream) {
#pragma omp atomic
                            finished++;
#pragma omp atomic
                            eval_lines++;
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
                        fprintf(stderr,"skipping line: %s (bad text line)\n",line_path);
                        continue;
                    } catch(const char *error) {
                        fprintf(stderr,"skipping line: %s (%s)\n",line_path,error);
                        if(abort_on_error) abort();
                        continue;
                    } catch(...) {
                        fprintf(stderr,"skipping line: %s (unknown exception)\n",line_path);
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
                        debugf("info","ERROR in bestpath: %s\n",error);
                        if(abort_on_error) abort();
                        continue;
                    } catch(...) {
                        debugf("info","ERROR in bestpath\n",error);
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
                    fprintf(stderr,"ERROR in recognizeLine: %s\n",error);
                    if(abort_on_error) abort();
                    continue;
                } catch(...) {
                    fprintf(stderr,"ERROR in recognizeLine\n");
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

    int main_pages2images(int argc,char **argv) {
        throw Unimplemented();
    }

    int main_pages2lines(int argc,char **argv) {
        if(argc!=2) throw "usage: ... dir";
        dinit(1000,1000);
        const char *outdir = argv[1];
        autodel<ISegmentPage> segmenter;

        autodel<IBookStore> bookstore;
        make_component(bookstore,cbookstore);
        bookstore->setPrefix(outdir);
        int npages = bookstore->numberOfPages();
        debugf("info","found %d pages\n",npages);
        if(npages<1) throw "no pages found";
#pragma omp parallel for private(segmenter)
        for(int pageno=0;pageno<npages;pageno++) {
            bytearray page_gray,page_binary;
#pragma omp critical
            {
                if(!segmenter) segmenter = make_SegmentPageByRAST();
                if(!bookstore->getPage(page_gray,pageno)) {
                    if(pageno>0)
                        debugf("info","%d: page not found\n",pageno);
                }
                if(!bookstore->getPage(page_binary,pageno,"bin")) {
                    page_binary = page_gray;
                }
            }
            if(page_gray.length()<1) continue;

            intarray page_seg;
            segmenter->segment(page_seg,page_binary);

#pragma omp critical
            {
                bookstore->putPage(page_seg,pageno,"pseg");
                RegionExtractor regions;
                regions.setPageLines(page_seg);
                for(int lineno=1;lineno<regions.length();lineno++) {
                    try {
                        bytearray line_image;
                        regions.extract(line_image,page_gray,lineno,1);
                        CHECK_ARG(line_image.dim(1)<maxheight);
                        CHECK_ARG(line_image.dim(1)*1.0/line_image.dim(0)<maxaspect);
                        // TODO/mezhirov output log of coordinates
                        // here
                        int id = regions.id(lineno);
                        if(!strcmp(cbookstore,"OldBookStore")) id = lineno;
                        bookstore->putLine(line_image,pageno,id);
                    } catch(const char *s) {
                        fprintf(stderr,"ERROR: %s\n",s);
                        if(abort_on_error) abort();
                    } catch(BadTextLine &err) {
                        fprintf(stderr,"ERROR: BadTextLine returned by recognizer\n");
                        if(abort_on_error) abort();
                    } catch(...) {
                        fprintf(stderr,"ERROR: (no details)\n");
                        if(abort_on_error) abort();
                    }
                }
                debugf("info","%4d: #lines = %d\n",pageno,regions.length());
                // TODO/mezhirov output other blocks here
            }
        }
        return 0;
    }


    int main_page(int argc,char **argv) {
        // create the segmenter
        autodel<ISegmentPage> segmenter;
        segmenter = make_SegmentPageByRAST();
        // load the line recognizer
        autodel<IRecognizeLine> linerec;
        linerec_load(linerec,cmodel);
        // load the language model
        autodel<OcroFST> langmod(make_OcroFST());
        try {
            langmod->load(lmodel);
        } catch(const char *s) {
            throwf("%s: failed to load (%s)",(const char*)lmodel,s);
        } catch(...) {
            throwf("%s: failed to load language model",(const char*)lmodel);
        }
        // now iterate through the pages
        for(int arg=1;arg<argc;arg++) {
            Pages pages;
            pages.parseSpec(argv[arg]);
            while(pages.nextPage()) {
                bytearray page_binary,page_gray;
                intarray page_seg;
                pages.getBinary(page_binary);
                pages.getGray(page_gray);
                segmenter->segment(page_seg,page_binary);
                RegionExtractor regions;
                regions.setPageLines(page_seg);
                for(int i=1;i<regions.length();i++) {
                    try {
                        bytearray line_image;
                        regions.extract(line_image,page_gray,i,1);
                        autodel<OcroFST> result(make_OcroFST());
                        linerec->recognizeLine(*result,line_image);
                        ustrg str;
                        if(0) {
                            result->bestpath(str);
                        } else {
                            double cost = beam_search(str,*result,*langmod,beam_width);
                            if(cost>1e10) throw "beam search failed";
                        }
                        utf8strg utf8Output;
                        str.utf8EncodeTerm(utf8Output);
                        printf("%s\n",utf8Output.c_str());
                    } catch(const char *error) {
                        fprintf(stderr,"[%s]\n",error);
                    }
                }
            }
        }
        return 0;
    }

    int main_recognize1(int argc,char **argv) {
        if(argc<3) throwf("usage: %s %s model image ...",command,argv[0]);
        if(!getenv("ocrolog"))
            throwf("please set ocrolog=glr in the environment");
        if(!getenv("ocrologdir"))
            throwf("please set ocrologdir to the target directory for the log");
        Logger logger("glr");
        dinit(512,512);
        autodel<IRecognizeLine> linerecp(make_Linerec());
        IRecognizeLine &linerec = *linerecp;
        stdio model(argv[1],"r");
        linerec.load(model);
        for(int i=2;i<argc;i++) {
            logger.html("<hr><br>");
            logger.format("Recognizing %s",argv[i]);
            bytearray image;
            read_image_gray(image,argv[i]);
            autodel<IGenericFst> result(make_OcroFST());
            linerec.recognizeLine(*result,image);

            // dump the corresponding FST for display
            // print_fst_simple(*result);
            // result->save("rec_temp.fst");
            dump_fst("rec_temp.dot",*result);
            intarray fst_image;
            fst_to_image(fst_image,*result);
            logger.log("fst",fst_image);

            // output the result
            ustrg str;
            str.clear();
            result->bestpath(str);
            if(str.length()<1) throw "recognition failed";
            if(debug("detail")) {
                debugf("detail","ustrg result: ");
                for(int i=0;i<str.length();i++)
                    printf(" %d",str(i).ord());
                printf("\n");
            }
            utf8strg utf8;
            str.utf8EncodeTerm(utf8);
            printf("%s\t%s\n", argv[i], utf8.c_str());
            // log the string to the output file
            char buffer[10000];
            sprintf(buffer,"<font style='color: green; font-size: 36pt;'>%s</font>\n",utf8.c_str());
            //logger.log("result",&s[0]);
            logger.html(buffer);
        }
        return 0;
    }

    int main_loadseg(int argc,char **argv) {
        if(argc!=3) throw "usage: ... model dir";
        dinit(512,512);
        autodel<IRecognizeLine> linerecp(make_Linerec());
        IRecognizeLine &linerec = *linerecp;
        struct stat sbuf;
        if(argv[1][0]!='.' && !stat(argv[1],&sbuf))
            throw "output model file already exists; please remove first";
        fprintf(stderr,"loading %s\n",argv[2]);
        linerec.startTraining("");
        linerec.set("load_ds8",argv[2]);
        linerec.finishTraining();
        fprintf(stderr,"saving %s\n",argv[1]);
        stdio stream(argv[1],"w");
        // linerec.save(stream);
        save_component(stream,&linerec);
        return 0;
    }

    int main_trainseg_or_saveseg(int argc,char **argv) {
        if(argc!=3) throw "usage: ... model dir";
        dinit(512,512);
        autodel<IRecognizeLine> linerecp(make_Linerec());
        IRecognizeLine &linerec = *linerecp;
        autodel<IBookStore> bookstore;
        make_component(bookstore,cbookstore);
        bookstore->setPrefix(argv[2]);
        CHECK(bookstore->numberOfPages()>0);
        int nfiles = 0;
        for(int page=0;page<bookstore->numberOfPages();page++)
            nfiles += bookstore->linesOnPage(page);
        CHECK(nfiles>0);
        intarray cseg;
        bytearray image;
        int total_chars = 0;
        int total_lines = 0;
        linerec.startTraining("");
        strg cseg_variant = "cseg.gt";
        strg text_variant = "gt";
        if(retrain) {
            cseg_variant = "cseg";
            text_variant = "";
        }
        int next = 1000;
        floatarray costs;
        for(int pageno=0;pageno<bookstore->numberOfPages();pageno++) {
            for(int lineno=0;lineno<bookstore->linesOnPage(pageno);lineno++) {
                int line = bookstore->getLineId(pageno,lineno);
                try {
                    if(!bookstore->getLine(cseg,pageno,line,cseg_variant)) {
                        debugf("info","%04d %06x: no such cseg\n",pageno,line);
                        continue;
                    }
                    make_line_segmentation_black(cseg);
                    CHECK(cseg.length()>100);
                    image.makelike(cseg);
                    for(int i=0;i<image.length1d();i++)
                        image.at1d(i) = 255*!cseg.at1d(i);

                    // read the ground truth segmentation

                    ustrg nutranscript;
                    {
                        bookstore->getLine(nutranscript,pageno,line,text_variant);
                        // FIXME this is an awful hack and won't work with unicode
                        char transcript[10000];
                        for(int i=0;i<nutranscript.length();i++)
                            transcript[i] = nutranscript[i].ord();
                        transcript[nutranscript.length()] = 0;
                        chomp(transcript);
                        if(old_csegs) remove_spaces(transcript);
                        nutranscript.assign(transcript);
                        if(nutranscript.length()!=max(cseg)) {
                            debugf("debug","transcript = '%s'\n",transcript);
                            throwf("transcript doesn't agree with cseg (transcript %d, cseg %d)",
                                   nutranscript.length(),max(cseg));
                        }
                    }

                    // for retraining, read the cost file

                    if(retrain) {
                        costs.resize(10000) = 1e38;
                        stdio stream(bookstore->path(pageno,line,"","costs"),"r");
                        int index;
                        float cost;
                        while(fscanf(stream,"%d %g\n",&index,&cost)==2) {
                            costs(index) = cost;
                        }
                        // remove all segments whose costs are too high
                        int old_length = nutranscript.length();
                        for(int i=0;i<nutranscript.length();i++) {
                            if(costs[i]>retrain_threshold)
                                nutranscript[i] = nuchar(' ');
                        }
                        for(int i=0;i<cseg.length();i++) {
                            if(costs(cseg[i])>retrain_threshold)
                                cseg[i] = 0;
                        }
                        int delta = old_length - nutranscript.length();
                        if(delta>0) {
                            debugf("info","removed %d exceeding cost\n",delta);
                        }
                        debugf("dcost","--------------------------------\n");
                        for(int i=0;i<nutranscript.length();i++) {
                            debugf("dcost","%3d %10g %c\n",i,costs(i),nutranscript(i).ord());
                        }
                        {
                            dsection("dcost");
                            dshowr(cseg);
                            dwait();
                        }
                    }

                    // let the user know about progress

                    {
                        utf8strg utf8Transcript;
                        nutranscript.utf8EncodeTerm(utf8Transcript);
                        debugf("transcript","%04d %06x (%d) [%2d,%2d] %s\n",pageno,line,total_chars,
                               nutranscript.length(),max(cseg),utf8Transcript.c_str());
                    }
                    if(total_chars>=next) {
                        debugf("info","loaded %d chars, %s total\n",
                               total_chars,linerec.command("total"));
                        next += 1000;
                    }

                    // now, actually add the segmented characters to the line recognizer

                    try {
                        linerec.addTrainingLine(cseg,image,nutranscript);
                        total_chars += max(cseg);
                        total_lines++;
                    } catch(const char *s) {
                        fprintf(stderr,"ERROR: %s\n",s);
                    } catch(BadTextLine &err) {
                        fprintf(stderr,"ERROR: BadTextLine\n");
                    } catch(...) {
                        fprintf(stderr,"ERROR: (no details)\n");
                    }
                } catch(const char *msg) {
                    printf("%04d %06x: %s FIXME\n",pageno,line,msg);
                }
                if(total_chars>=ntrain) break;
            }
        }
        if(!strcmp(argv[0],"trainseg")) {
            linerec.finishTraining();
            fprintf(stderr,"trained %d characters, %d lines\n",
                    total_chars,total_lines);
            fprintf(stderr,"saving %s\n",argv[1]);
            stdio stream(argv[1],"w");
            // linerec.save(stream);
            save_component(stream,&linerec);
            return 0;
        } else if(!strcmp(argv[0],"saveseg")) {
            fprintf(stderr,"saving %d characters, %d lines\n",
                    total_chars,total_lines);
            fprintf(stderr,"saving %s\n",argv[1]);
            linerec.set("save_ds8",argv[1]);
            // stdio stream(argv[1],"w");
            // linerec.save(stream);
            return 0;
        } else throw "oops";
    }

    int main_bookstore(int argc,char **argv) {
        autodel<IBookStore> bookstore;
        make_component(bookstore,cbookstore);
        bookstore->setPrefix(argv[1]);
        int npages = bookstore->numberOfPages();
        printf("#pages %d\n",npages);
        for(int i=0;i<npages;i++) {
            int nlines = bookstore->linesOnPage(i);
            printf("%d %d\n",i,nlines);
        }
        return 0;
    }

    void usage(const char *program) {
        fprintf(stderr,"usage:\n");
#define D(s,s2) {fprintf(stderr,"    %s %s\n",program,s); fprintf(stderr,"        %s\n",s2); }
#define P(s) {fprintf(stderr,"%s\n",s);}
#define SECTION(x) {fprintf(stderr,"\n*** %s\n\n",x);}
        SECTION("splitting books");
        D("book2pages dir image image ...",
                "convert the input into a set of pages under dir/...");
        D("pages2lines dir",
                "convert the pages in dir/... into lines");
        SECTION("line recognition and language modeling")
        D("lines2fsts dir",
                    "convert the lines in dir/... into fsts (lattices); cmodel=...")
        D("fsts2bestpaths dir",
                "find the best interpretation of the fsts in dir/... without a language model");
        D("fsts2textdir",
                "find the best interpretation of the fsts in dir/...; lmodel=...");
        SECTION("evaluation");
        D("evaluate dir",
                "evaluate the quality of the OCR output in dir/...");
        D("evalconf dir",
                "evaluate the quality of the OCR output in dir/... and outputs confusion matrix");
        D("findconf dir from to",
                "finds instances of confusion of from to to (according to edit distance)");
        D("evaluate1 file1 file2",
                "compute the edit distance between the two files");
        SECTION("training");
        D("align dir",
                "align fsts with ground truth transcripts");
        D("trainseg model dir",
                "train a model for the ground truth in dir/...");
        D("saveseg dataset dir",
                "perform dataset extraction on the book directory and save it");
        D("loadseg model dataset",
                "perform training on the dataset (saveseg + loadseg is the same as trainseg)");
        SECTION("other recognizers");
        D("recognize1 logdir model line1 line2...",
                "recognize images of individual lines of text given on the command line; ocrolog=glr ocrologdir=...");
        D("page image.png",
                "recognize a single page of text without adaptivity, but with a language model");
        SECTION("components");
        D("components",
                "list available components (of any type)");
        D("params component",
                "output the available parameters for the given component");
        D("cinfo model",
                "load the classifier model and print information on it");
        SECTION("results");
        D("buildhtml dir",
                "creates an HTML representation of the OCR output in dir/...");
#if 0
        D("cleanhtml dir",
                "removes all files from dir/... that aren't needed for the HTML output");
#endif
        exit(1);
    }

    extern int main_buildhtml(int argc,char **argv);
    extern int main_cleanhtml(int argc,char **argv);
    extern int main_evaluate(int argc,char **argv);
    extern int main_evalconf(int argc,char **argv);
    extern int main_findconf(int argc,char **argv);
    extern int main_evalfiles(int argc,char **argv);
    extern int main_components(int argc,char **argv);
    extern int main_cinfo(int argc,char **argv);
    extern int main_linfo(int argc,char **argv);
    extern int main_params(int argc,char **argv);
    extern int main_align(int argc,char **argv);
    extern int main_fsts2text(int argc,char **argv);
    extern int main_fsts2bestpaths(int argc,char **argv);

    int main_ocropus(int argc,char **argv) {
#ifdef DLOPEN
        if(extension) {
            void *handle = dlopen(extension,RTLD_LAZY);
            if(!handle) {
                fprintf(stderr,"%s: cannot load\n",dlerror());
                exit(1);
            }
            void (*init)() = (void(*)())dlsym(handle,"ocropus_init_dl");
            if(dlerror()!=NULL) {
                fprintf(stderr,"%s: cannot init library\n",dlerror());
                exit(1);
            }
            init();
        }
#endif
        try {
            command = argv[0];
            init_ocropus_components();
            init_glclass();
            init_glfmaps();
            init_linerec();
            if(argc<2) usage(argv[0]);
            if(!strcmp(argv[1],"threshold")) return main_threshold(argc-1,argv+1);
            if(!strcmp(argv[1],"book2pages")) return main_book2pages(argc-1,argv+1);
            if(!strcmp(argv[1],"buildhtml")) return main_buildhtml(argc-1,argv+1);
            if(!strcmp(argv[1],"cinfo")) return main_cinfo(argc-1,argv+1);
            if(!strcmp(argv[1],"linfo")) return main_linfo(argc-1,argv+1);
            if(!strcmp(argv[1],"cleanhtml")) return main_buildhtml(argc-1,argv+1);
            if(!strcmp(argv[1],"components")) return main_components(argc-1,argv+1);
            if(!strcmp(argv[1],"evalconf")) return main_evalconf(argc-1,argv+1);
            if(!strcmp(argv[1],"evaluate")) return main_evaluate(argc-1,argv+1);
            if(!strcmp(argv[1],"evaluate1")) return main_evalfiles(argc-1,argv+1);
            if(!strcmp(argv[1],"findconf")) return main_findconf(argc-1,argv+1);
            if(!strcmp(argv[1],"fsts2bestpaths")) return main_fsts2bestpaths(argc-1,argv+1);
            if(!strcmp(argv[1],"fsts2text")) return main_fsts2text(argc-1,argv+1);
            if(!strcmp(argv[1],"lines2fsts")) return main_lines2fsts(argc-1,argv+1);
            if(!strcmp(argv[1],"loadseg")) return main_loadseg(argc-1,argv+1);
            if(!strcmp(argv[1],"align")) return main_align(argc-1,argv+1);
            if(!strcmp(argv[1],"page")) return main_page(argc-1,argv+1);
            if(!strcmp(argv[1],"pages2images")) return main_pages2images(argc-1,argv+1);
            if(!strcmp(argv[1],"pages2lines")) return main_pages2lines(argc-1,argv+1);
            if(!strcmp(argv[1],"params")) return main_params(argc-1,argv+1);
            if(!strcmp(argv[1],"recognize1")) return main_recognize1(argc-1,argv+1);
            if(!strcmp(argv[1],"saveseg")) return main_trainseg_or_saveseg(argc-1,argv+1);
            if(!strcmp(argv[1],"trainseg")) return main_trainseg_or_saveseg(argc-1,argv+1);
            if(!strcmp(argv[1],"bookstore")) return main_bookstore(argc-1,argv+1);
            usage(argv[0]);
        } catch(const char *s) {
            fprintf(stderr,"FATAL: %s\n",s);
        }
        return 0;
    }
}
