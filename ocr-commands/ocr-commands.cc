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

namespace glinerec {
    const char *command = "";
}

namespace ocropus {
    using namespace iulib;
    using namespace colib;
    using namespace ocropus;
    using namespace narray_ops;
    using namespace glinerec;

    const char *exception_context = "???";

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

    void chomp_extension(char *s) {
        char *p = s+strlen(s);
        while(p>s) {
            --p;
            if(*p=='/') break;
            if(*p=='.') *p = 0;
        }
    }

    void linerec_load(autodel<IRecognizeLine> &linerec,const char *cmodel) {
#if LOAD_OBSOLETE_FORMATS_OPTIONALLY
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
#else
        load_component(stdio(cmodel,"r"),linerec);
#endif
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
        param_string cbookstore("bookstore","SmartBookStore","storage abstraction for book");
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


    int main_pages2images(int argc,char **argv) {
        throw Unimplemented();
    }

    int main_pages2lines(int argc,char **argv) {
        param_bool abort_on_error("abort_on_error",0,"abort recognition if there is an unexpected error");
        param_string cbookstore("bookstore","SmartBookStore","storage abstraction for book");
        param_int extract_grow("extract_grow",1,"amount by which to grow the mask for line extractions (-1=no mask)");
        param_float maxheight("max_line_height",300,"maximum line height");
        param_float maxaspect("max_line_aspect",1.0,"maximum line aspect ratio");
        param_string csegmenter("psegmenter","SegmentPageByRAST","segmenter to use at the page level");
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
            try {
                if(!segmenter) {
                    make_component(csegmenter,segmenter);
                }
                if(!bookstore->getPage(page_gray,pageno)) {
                    if(pageno>0)
                        debugf("warn","%d: page not found\n",pageno);
                }
                if(!bookstore->getPage(page_binary,pageno,"bin")) {
                    page_binary = page_gray;
                }
            } catch(const char *s) {
                debugf("error","page %d: %s\n",pageno,s);
                if(abort_on_error) abort();
            } catch(...) {
                debugf("error","page %d (no details)\n",pageno);
                if(abort_on_error) abort();
            }

            if(page_gray.length()<1) continue;

            intarray page_seg;
            try {
                segmenter->segment(page_seg,page_binary);
            } catch(const char *s) {
                fprintf(stderr,"%s: segmentation of page %d\n",s,pageno);
                if(abort_on_error) abort();
            } catch(...) {
                fprintf(stderr,"error in segmentation of page %d\n",pageno);
                if(abort_on_error) abort();
            }

#pragma omp critical
            try {
                bookstore->putPage(page_seg,pageno,"pseg");
                RegionExtractor regions;
                regions.setPageLines(page_seg);
                int grow = extract_grow;
                for(int lineno=1;lineno<regions.length();lineno++) {
                    try {
                        bytearray line_image;
                        if(extract_grow<0) {
                            regions.extract(line_image,page_gray,lineno,1);
                        } else {
                            regions.extract_masked(line_image,page_gray,lineno,(byte)grow,255,1);
                        }
                        CHECK_ARG(line_image.dim(1)<maxheight);
                        CHECK_ARG(line_image.dim(1)*1.0/line_image.dim(0)<maxaspect);
                        int id = regions.id(lineno);
                        if(!strcmp(cbookstore,"OldBookStore")) id = lineno;
                        bookstore->putLine(line_image,pageno,id);
                    } catch(const char *s) {
                        debugf("error","%s: page %d line %d\n",s,pageno,lineno);
                        if(abort_on_error) abort();
                    } catch(...) {
                        debugf("error","page %d line %d\n",pageno,lineno);
                        if(abort_on_error) abort();
                    }
                }
                debugf("info","%4d: #lines = %d\n",pageno,regions.length()-1);
                // TODO/mezhirov output other blocks here
            } catch(const char *s) {
                debugf("error","%s: page %d\n",s,pageno);
                if(abort_on_error) abort();
            } catch(...) {
                debugf("error","error in page %d\n",pageno);
                if(abort_on_error) abort();
            }
        }
        return 0;
    }


    int main_page(int argc,char **argv) {
        param_int beam_width("beam_width", 100, "number of nodes in a beam generation");
        param_string csegmenter("csegmenter","SegmentPageByRAST","page segmentation component");
        param_string cmodel("cmodel",DEFAULT_DATA_DIR "default.model","character model used for recognition");
        param_string lmodel("lmodel",DEFAULT_DATA_DIR "default.fst","language model used for recognition");
        // create the segmenter
        autodel<ISegmentPage> segmenter;
        make_component(segmenter,csegmenter);
        // load the line recognizer
        autodel<IRecognizeLine> linerec;
        linerec_load(linerec,cmodel);
        // load the language model
        autodel<OcroFST> langmod;
        if(lmodel && strcmp(lmodel,"")) {
            langmod = make_OcroFST();
            try {
                langmod->load(lmodel);
            } catch(const char *s) {
                throwf("%s: failed to load (%s)",(const char*)lmodel,s);
            } catch(...) {
                throwf("%s: failed to load language model",(const char*)lmodel);
            }
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
                        if(!langmod) {
                            result->bestpath(str);
                        } else {
                            double cost = beam_search(str,*result,*langmod,beam_width);
                            if(cost>1e10) throw "beam search failed";
                        }
                        utf8strg utf8Output;
                        str.utf8EncodeTerm(utf8Output);
                        printf("%s\n",utf8Output.c_str());
                    } catch(const char *error) {
                        debugf("error","%s\n",error);
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

    struct LineSource : IComponent {
        narray< autodel<IBookStore> > bookstores;
        int index;
        strg cseg_variant;
        strg text_variant;
        intarray all_lines;

        LineSource() {
            pdef("retrain",0,"set parameters for retraining");
            pdef("randomize",1,"randomize lines");
            pdef("cbookstore","SmartBookStore","bookstore using for reading pages");
        }
        const char *name() {
            return "linesource";
        }

        void init(char **books) {
            bool retrain = pgetf("retrain");
            bool randomize = pgetf("randomize");
            const char *cbookstore = pget("cbookstore");
            bookstores.clear();
            all_lines.clear();
            cseg_variant = "cseg.gt";
            text_variant = "gt";
            if(retrain) {
                cseg_variant = "cseg";
                text_variant = "";
            }

            int nbooks = 0;
            while(books[nbooks]) nbooks++;
            bookstores.resize(nbooks);
            for(int i=0;books[i];i++) {
                make_component(bookstores[i],cbookstore);
                bookstores[i]->setPrefix(books[i]);
                CHECK(bookstores[i]->numberOfPages()>0);
                debugf("info","%s: %d pages\n",books[i],bookstores[i]->numberOfPages());
            }

            // compute a list of all lines

            intarray triple(3);
            for(int i=0;i<nbooks;i++) {
                for(int j=0;j<bookstores[i]->numberOfPages();j++) {
                    for(int k=0;k<bookstores[i]->linesOnPage(j);k++) {
                        triple[0] = i;
                        triple[1] = j;
                        triple[2] = k;
                        rowpush(all_lines,triple);
                    }
                }
            }
            debugf("info","got %d lines\n",all_lines.dim(0));

            // randomly permute it so that we train in random order

            if(randomize) {
                intarray permutation;
                for(int i=0;i<all_lines.dim(0);i++) permutation.push(i);
                randomly_permute(permutation);
                rowpermute(all_lines,permutation);
            }

            index = -1;
        }

        bool done() {
            return index+1>=all_lines.dim(0);
        }

        int bookno;
        int pageno;
        int lineno_;
        int lineno;

        void next() {
            index++;
            bookno = all_lines(index,0);
            pageno = all_lines(index,1);
            lineno_ = all_lines(index,2);
            lineno = bookstores[bookno]->getLineId(pageno,lineno_);
        }

        void get(strg &str,const char *variant) {
            CHECK(!strcmp(variant,"path"));
            str.clear();
            str = bookstores[bookno]->path(pageno,lineno,0,0);
        }

        void get(ustrg &str,const char *variant) {
            str.clear();
            CHECK(!strcmp(variant,"transcript"));
            str.clear();
            bookstores[bookno]->getLine(str,pageno,lineno,"gt");
            if(str.length()>0) return;
            bookstores[bookno]->getLine(str,pageno,lineno,0);
            if(str.length()>0) return;
            throw "cannot find either .gt.txt or .txt file";
        }

        void get(bytearray &image,const char *variant) {
            image.clear();
            CHECK(!strcmp(variant,"image"));
            bookstores[bookno]->getLine(image,pageno,lineno,0);
        }

        void get(intarray &image,const char *variant) {
            image.clear();
            CHECK(!strcmp(variant,"cseg"));
            image.clear();
            bookstores[bookno]->getLine(image,pageno,lineno,"cseg.gt");
            if(image.length()>0) return;
            bookstores[bookno]->getLine(image,pageno,lineno,"cseg");
            if(image.length()>0) return;
            throw "cannot find either .cseg.gt.png or .cseg.png";
        }

        void get(floatarray &costs,const char *variant) {
            costs.clear();
            CHECK(!strcmp(variant,"costs"));
            costs.resize(10000) = 1e38;
            stdio stream(bookstores[bookno]->path(pageno,lineno,0,"costs"),"r");
            int index;
            float cost;
            while(fscanf(stream,"%d %g\n",&index,&cost)==2) {
                costs(index) = cost;
            }
        }
    };

    void remove_high_cost_segments(ustrg &nutranscript,floatarray &costs,
                                   intarray cseg,float retrain_threshold) {
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

    void fixup_transcript(ustrg &nutranscript,bool old_csegs) {
        char transcript[10000];
        for(int i=0;i<nutranscript.length();i++)
            transcript[i] = nutranscript[i].ord();
        transcript[nutranscript.length()] = 0;
        chomp(transcript);
        if(old_csegs) remove_spaces(transcript);
        nutranscript.assign(transcript);
    }

    int main_trainseg(int argc,char **argv) {
        param_int nepochs("nepochs",1,"number of epochs");
        param_bool randomize("randomize_lines",1,"randomize the order of lines before training");
        param_string cbookstore("bookstore","SmartBookStore","storage abstraction for book");
        param_bool retrain("retrain",0,"perform retraining");
        param_bool retrain_threshold("retrain_threshold",100,"only retrain on characters with a cost lower than this");
        param_int ntrain("ntrain",10000000,"max number of training examples");
        param_bool old_csegs("old_csegs",0,"(obsolete, old vs new csegs is now determined automatically)");
        int nold_csegs = 0;

        if(argc!=3) throw "usage: ... model books...";

        dinit(512,512);
        autodel<IRecognizeLine> linerec;
        linerec = make_Linerec();
        if(linerec) linerec->startTraining("");
        bool done = 0;

        int total_chars = 0;
        int total_lines = 0;
        int next = 1000;

        for(int epoch=0;epoch<nepochs;epoch++) {
            linerec->epoch(epoch);

            LineSource lines;
            char **books = argv+2;
            lines.init(books);

            while(!lines.done()) {
                lines.next();

                strg path;
                intarray cseg;
                bytearray image;
                floatarray costs;
                ustrg nutranscript;

                try {
                    lines.get(path,"path");
                    lines.get(image,"image");
                    if(image.length()==0) throw "no or bad line image";
                    lines.get(nutranscript,"transcript");
                    if(nutranscript.length()==0) throw "no transcription";
                    lines.get(cseg,"cseg");
                    if(cseg.length()==0) throw "no or bad cseg";
                } catch(const char *s) {
                    debugf("warn","skipping %s (%s)\n",path.c_str(),s);
                    continue;
                } catch(...) {
                    debugf("warn","skipping %s\n",path.c_str());
                    continue;
                }

                try {
                    make_line_segmentation_black(cseg);
                    image.makelike(cseg);
                    for(int i=0;i<image.length1d();i++)
                        image.at1d(i) = 255*!cseg.at1d(i);

                    // try to convert it to see whether it's an old_cseg
                    // or new cseg

                    ustrg s;
                    s = nutranscript;
                    fixup_transcript(s,false);
                    bool old_csegs = (s.length()!=max(cseg));
                    if(old_csegs) nold_csegs++;

                    fixup_transcript(nutranscript,old_csegs);

                    if(nutranscript.length()!=max(cseg)) {
                        debugf("debug","transcript = '%s'\n",nutranscript.c_str());
#if 1
                        debugf("warn","transcript doesn't agree with cseg (transcript %d, cseg %d)\n",
                               nutranscript.length(),max(cseg));
                        continue;
#else
                        throwf("transcript doesn't agree with cseg (transcript %d, cseg %d)",
                               nutranscript.length(),max(cseg));
#endif
                    }

                    if(retrain) {
                        lines.get(costs,"costs");
                        remove_high_cost_segments(nutranscript,costs,cseg,retrain_threshold);
                    }

                    // let the user know about progress

                    utf8strg utf8Transcript;
                    nutranscript.utf8EncodeTerm(utf8Transcript);
                    debugf("transcript","%04d %06x (%d) [%2d,%2d] %s\n",
                           lines.pageno,lines.lineno,total_chars,
                           nutranscript.length(),max(cseg),utf8Transcript.c_str());

                    if(total_chars>=next) {
                        debugf("info","loaded %d chars\n",total_chars);
                        next += 1000;
                    }

                    // now, actually add the segmented characters to the line recognizer

                    try {
                        linerec->addTrainingLine(cseg,image,nutranscript);
                        total_chars += nutranscript.length();
                        total_lines++;
                    } catch(DoneTraining _) {
                        done = 1;
                        break;
                    } CATCH_COMMON(continue);
                } catch(const char *msg) {
                    debugf("error","%04d %06x: %s\n",lines.pageno,lines.lineno,msg);
                }
                // if we have enough characters, let the loop wind down
                if(total_chars>ntrain) break;
            }
            if(done) break;
        }
        linerec->finishTraining();
        debugf("info","trained %d characters, %d lines\n",
                total_chars,total_lines);
        if(nold_csegs>total_lines/100)
            debugf("warn","%d old csegs\n",nold_csegs);
        debugf("info","saving %s\n",argv[1]);
        save_component(stdio(argv[1],"w"),linerec);
        return 0;
    }

    int main_trainmodel(int argc,char **argv) {
        param_string cmodel("cmodel","latin","classifier component");
        param_string cdataset("cdataset","rowdataset8","dataset component");
        if(argc!=3) throw "usage: ... model dataset";
        if(file_exists(argv[1])) throwf("%s: already exists",argv[1]);
        autodel<IDataset> ds;
        make_component(cdataset,ds);
        ds->load(argv[2]);
        debugf("info","%d nsamples, %d nfeatures, %d nclasses\n",
               ds->nsamples(),ds->nfeatures(),ds->nclasses());
        autodel<IModel> model;
        make_component(cmodel,model);
        model->xtrain(*ds);
        save_component(stdio(argv[1],"w"),model);
        return 0;
    }

    int main_bookstore(int argc,char **argv) {
        param_string cbookstore("bookstore","SmartBookStore","storage abstraction for book");
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

    int main_cleanup(int argc,char **argv) {
        param_string pclean("cleanup","StandardPreprocessing","cleanup component");
        autodel<IBinarize> cleanup;
        make_component(pclean,cleanup);
        bytearray in,out;
        read_image_gray(in,argv[1]);
        cleanup->binarize(out,in);
        write_image_gray(argv[2],out);
        return 0;
    }

    int main_cleanupbin(int argc,char **argv) {
        param_string pclean("cleanup","StandardPreprocessing","cleanup component");
        autodel<ICleanupBinary> cleanup;
        make_component(pclean,cleanup);
        bytearray in,out;
        read_image_gray(in,argv[1]);
        cleanup->cleanup(out,in);
        write_image_gray(argv[2],out);
        return 0;
    }

    int main_cleanupgray(int argc,char **argv) {
        param_string pclean("cleanup","StandardPreprocessing","cleanup component");
        autodel<ICleanupGray> cleanup;
        make_component(pclean,cleanup);
        bytearray in,out;
        read_image_gray(in,argv[1]);
        cleanup->cleanup_gray(out,in);
        write_image_gray(argv[2],out);
        return 0;
    }

    int main_recolor(int argc,char **argv) {
        intarray segmentation;
        read_image_packed(segmentation,argv[1]);
        for(int i=0;i<segmentation.length();i++)
            if(segmentation[i]==0xffffff) segmentation[i] = 0;
        simple_recolor(segmentation);
        write_image_packed(argv[2],segmentation);
        return 0;
    }

    int main_show(int argc,char **argv) {
        bytearray image;
        read_image_gray(image,argv[1]);
        dinit(min(image.dim(0),1500),
              min(image.dim(1),1100),true);
        dshown(image);
        dwait();
        return 0;
    }

    int main_showseg(int argc,char **argv) {
        intarray segmentation;
        read_image_packed(segmentation,argv[1]);
        dinit(min(segmentation.dim(0),1500),
              min(segmentation.dim(1),1100),true);
        for(int i=0;i<segmentation.length();i++)
            if(segmentation[i]==0xffffff) segmentation[i] = 0;
        dshowr(segmentation);
        dwait();
        return 0;
    }

    int main_pageseg(int argc,char **argv) {
        param_bool recolor("recolor",0,"recolor segmentation");
        param_string csegmenter("psegmenter","SegmentPageByRAST","segmenter to use at the page level");
        bytearray image;
        intarray segmentation;
        read_image_gray(image,argv[1]);
        autodel<ISegmentPage> segmenter;
        make_component(csegmenter,segmenter);
        segmenter->segment(segmentation,image);
        if(recolor) {
            make_page_segmentation_black(segmentation);
            simple_recolor(segmentation);
        }
        write_image_packed(argv[2],segmentation);
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

    void load_extensions(const char *dir) {
#ifdef DLOPEN
        strg pattern;
        pattern = dir;
        pattern += "/*.so";
        Glob glob(pattern);
        for(int i=0;i<glob.length();i++) {
            const char *so = glob(i);
            debugf("info","loading %s\n",so);
            void *handle = dlopen(so,RTLD_LAZY);
            if(!handle) {
                fprintf(stderr,"%s: cannot load\n",dlerror());
                exit(1);
            }
            void (*init)() = (void(*)())dlsym(handle,"ocropus_init_dl");
            const char *err = dlerror();
            if(err!=NULL) {
                fprintf(stderr,"%s: cannot init library\n",err);
                exit(1);
            }
            init();
        }
#endif
    }

    int main_ocropus(int argc,char **argv) {
        try {
            command = argv[0];
            init_ocropus_components();
            init_glclass();
            init_glfmaps();
            init_linerec();
            load_extensions(DEFAULT_EXT_DIR);
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
            extern int main_lines2fsts(int,char **);
            if(!strcmp(argv[1],"lines2fsts")) return main_lines2fsts(argc-1,argv+1);
            if(!strcmp(argv[1],"trainmodel")) return main_trainmodel(argc-1,argv+1);
            if(!strcmp(argv[1],"align")) return main_align(argc-1,argv+1);
            if(!strcmp(argv[1],"page")) return main_page(argc-1,argv+1);
            if(!strcmp(argv[1],"pages2images")) return main_pages2images(argc-1,argv+1);
            if(!strcmp(argv[1],"pages2lines")) return main_pages2lines(argc-1,argv+1);
            if(!strcmp(argv[1],"params")) return main_params(argc-1,argv+1);
            if(!strcmp(argv[1],"recognize1")) return main_recognize1(argc-1,argv+1);
            if(!strcmp(argv[1],"trainseg")) return main_trainseg(argc-1,argv+1);
            if(!strcmp(argv[1],"bookstore")) return main_bookstore(argc-1,argv+1);
            if(!strcmp(argv[1],"cleanup")) return main_cleanup(argc-1,argv+1);
            if(!strcmp(argv[1],"cleanupgray")) return main_cleanupgray(argc-1,argv+1);
            if(!strcmp(argv[1],"cleanupbin")) return main_cleanupbin(argc-1,argv+1);
            if(!strcmp(argv[1],"recolor")) return main_recolor(argc-1,argv+1);
            if(!strcmp(argv[1],"show")) return main_show(argc-1,argv+1);
            if(!strcmp(argv[1],"showseg")) return main_showseg(argc-1,argv+1);
            if(!strcmp(argv[1],"pageseg")) return main_pageseg(argc-1,argv+1);
            usage(argv[0]);
        } catch(const char *s) {
            fprintf(stderr,"FATAL: %s\n",s);
        }
        return 0;
    }
}
