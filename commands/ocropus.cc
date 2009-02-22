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
#include "ocr-openfst.h"

using namespace iulib;
using namespace colib;
using namespace ocropus;
using namespace narray_ops;

namespace glinerec {
    IRecognizeLine *make_Linerec();
}

void nustring_convert(narray<char> &output,nustring &str) {
    // FIXME the utf8Encode doesn't seem to be doing the right thing
    output.clear();
    for(int i=0;i<str.length();i++) {
        int c = str(i).ord();
        if(c>=32 && c<=127)
            output.push() = c;
        else
            output.push() = '~';
    }
    output.push() = 0;
}

void nustring_convert(nustring &output,strbuf &str) {
    // FIXME the utf8Decode doesn't seem to be doing the right thing
    int n = str.length();
    output.resize(n);
    for(int i=0;i<n;i++)
        output[i] = nuchar((int)str[i]);
}

void nustring_convert(strbuf &output,nustring &str) {
    // FIXME the utf8Encode doesn't seem to be
    // doing the right thing
    output.dealloc();
    for(int i=0;i<str.length();i++) {
        int c = str(i).ord();
        if(c>=32 && c<=127)
            output += c;
        else
            output += '~';
    }
}

struct Glob {
    glob_t g;
    Glob(const char *pattern,int flags=0) {
        glob(pattern,flags,0,&g);
    }
    ~Glob() {
        globfree(&g);
    }
    int length() {
        return g.gl_pathc;
    }
    const char *operator()(int i) {
        CHECK(i>=0 && i<length());
        return g.gl_pathv[i];
    }
};

int main_book2lines(int argc,char **argv) {
    int pageno = 0;
    autodel<ISegmentPage> segmenter;
    segmenter = make_SegmentPageByRAST();
    const char *outdir = argv[1];
    if(mkdir(outdir,0777)) {
        fprintf(stderr,"error creating OCR working directory\n");
        perror(outdir);
        exit(1);
    }
    strbuf s;
    for(int arg=2;arg<argc;arg++) {
        Pages pages;
        pages.parseSpec(argv[arg]);
        while(pages.nextPage()) {
            pageno++;
            debugf("info","page %d\n",pageno);
            s.format("%s/%04d",outdir,pageno);
            mkdir(s,0777);
            bytearray page_binary,page_gray;
            pages.getBinary(page_binary);
            pages.getGray(page_gray);
            intarray page_seg;
            segmenter->segment(page_seg,page_binary);
            RegionExtractor regions;
            regions.setPageLines(page_seg);
            for(int lineno=1;lineno<regions.length();lineno++) {
                bytearray line_image;
                regions.extract(line_image,page_gray,lineno,1);
                // FIXME output log of coordinates here
                s.format("%s/%04d/%04d.png",outdir,pageno,lineno);
                write_image_gray(s,line_image);
            }
            debugf("info","#lines = %d\n",regions.length());
            // FIXME output images here
        }
    }
    return 0;
}

int main_book2pages(int argc,char **argv) {
    int pageno = 0;
    const char *outdir = argv[1];
    if(mkdir(outdir,0777)) {
        fprintf(stderr,"error creating OCR working directory\n");
        perror(outdir);
        exit(1);
    }
    strbuf s;
    for(int arg=2;arg<argc;arg++) {
        Pages pages;
        pages.parseSpec(argv[arg]);
        while(pages.nextPage()) {
            pageno++;
            debugf("info","page %d\n",pageno);
            mkdir(s,0777);
            bytearray page_binary,page_gray;

            // FIXME make binarizer settable
            s.format("%s/%04d.png",outdir,pageno);
            pages.getGray(page_gray);
            write_image_gray(s,page_gray);

            pages.getBinary(page_binary);
            s.format("%s/%04d.bin.png",outdir,pageno);
            write_image_binary(s,page_binary);
        }
    }
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
    segmenter = make_SegmentPageByRAST();
    strbuf s;
    s.format("%s/[0-9][0-9][0-9][0-9].png",outdir);
    Glob files(s);
    if(files.length()<1)
        throw "no pages found";
    for(int index=0;index<files.length();index++) {
        char buf[1000];
        int pageno=9999;

        CHECK(sscanf(files(index),"%[^/]/%d.png",buf,&pageno)==2);
        debugf("info","page %d\n",pageno);

        s.format("%s/%04d",outdir,pageno);
        mkdir(s,0777);          // ignore errors

        bytearray page_gray;
        read_image_gray(page_gray,files(index));
        bytearray page_binary;
        s.format("%s/%04d.bin.png",outdir,pageno);
        read_image_binary(page_binary,s);

        intarray page_seg;
        segmenter->segment(page_seg,page_binary);

        RegionExtractor regions;
        regions.setPageLines(page_seg);
        for(int lineno=1;lineno<regions.length();lineno++) {
            bytearray line_image;
            regions.extract(line_image,page_gray,lineno,1);
            // FIXME output log of coordinates here
            s.format("%s/%04d/%04d.png",outdir,pageno,lineno);
            write_image_gray(s,line_image);
        }
        debugf("info","#lines = %d\n",regions.length());
        // FIXME output other blocks here
    }
    return 0;
}

param_bool abort_on_error("abort_on_error",1,"abort recognition if there is an unexpected error");

int main_tesslines(int argc,char **argv) {
    if(argc!=2) throw "usage: ... dir";
    dinit(1000,1000);
    autodel<IRecognizeLine> linerec;
    linerec = make_TesseractRecognizeLine();
    strbuf pattern;
    pattern.format("%s/[0-9][0-9][0-9][0-9]/[0-9][0-9][0-9][0-9].png",argv[1]);
    Glob files(pattern);
    for(int index=0;index<files.length();index++) {
        if(index%1000==0)
            debugf("info","%s (%d/%d)\n",files(index),index,files.length());
        strbuf base;
        base = files(index);
        base[base.length()-4] = 0;
        debugf("progress","line %s\n",(char*)base);
        bytearray image;
        // FIXME output binary versions, intermediate results for debugging
        read_image_gray(image,files(index));
        autodel<IGenericFst> result(make_StandardFst());
        try {
            linerec->recognizeLine(*result,image);
        }
#if 0
        catch(BadTextLine &error) {
            fprintf(stderr,"ERROR: %s (bad text line)\n",base.ptr());
            continue;
        }
#endif
        catch(const char *error) {
            fprintf(stderr,"ERROR: %s\n",error);
            if(abort_on_error) abort();
            continue;
        }
        nustring str;
        result->bestpath(str);
        if(debug("ccoding")) {
            debugf("ccoding","nustring:");
            for(int i=0;i<str.length();i++)
                printf(" %d[%c]",str(i).ord(),str(i).ord());
            printf("\n");
        }
        strbuf output;
        nustring_convert(output,str);
        strbuf s;
        s.format("%s.txt",(char*)base);
        fprintf(stdio(s,"w"),"%s\n",output.ptr());
        debugf("transcript","%s\t%s\n",files(index),output.ptr());
    }
    return 0;
}

param_string eval_flags("eval_flags","space","which features to ignore during evaluation");

void cleanup_for_eval(strbuf &s) {
    bool space = strflag(eval_flags,"space");
    bool scase = strflag(eval_flags,"case");
    bool nonanum = strflag(eval_flags,"nonanum");
    bool nonalpha = strflag(eval_flags,"nonalpha");
    strbuf result;
    result = "";
    for(int i=0;i<s.length();i++) {
        int c = s[i];
        if(space && c==' ') continue;
        if(nonanum && !isalnum(c)) continue;
        if(nonalpha && !isalpha(c)) continue;
        if(c<32||c>127) continue;
        if(scase && isupper(c)) c = tolower(c);
        result += c;
    }
    s = result;
}

int main_evaluate(int argc,char **argv) {
    if(argc!=2) throw "usage: ... dir";
    strbuf s;
    s.format("%s/[0-9][0-9][0-9][0-9]/[0-9][0-9][0-9][0-9].gt.txt",argv[1]);
    Glob files(s);
    float total = 0.0, tchars = 0, pchars = 0, lines = 0;
    for(int index=0;index<files.length();index++) {
        if(index%1000==0)
            debugf("info","%s (%d/%d)\n",files(index),index,files.length());

        strbuf base;
        base = files(index);
        base.truncate(-7);

        char buf[100000];
        try {
            fgets(buf,sizeof buf,stdio(files(index),"r"));
        } catch(const char *error) {
            continue;
        }
        strbuf truth;
        truth = buf;

        strbuf s; s = base; s += ".txt";
        try {
            fgets(buf,sizeof buf,stdio(s.ptr(),"r"));
        } catch(const char *error) {
            continue;
        }
        strbuf predicted;
        predicted = buf;

        cleanup_for_eval(truth);
        cleanup_for_eval(predicted);
        nustring ntruth,npredicted;
        nustring_convert(ntruth,truth);
        nustring_convert(npredicted,predicted);
        float dist = edit_distance(ntruth,npredicted);

        total += dist;
        tchars += truth.length();
        pchars += predicted.length();
        lines++;

        debugf("transcript",
               "%g\t%s\t%s\t%s\n",
               dist,
               files(index),
               truth.ptr(),
               predicted.ptr());
    }
    printf("rate %g total_error %g true_chars %g predicted_chars %g lines %g\n",
           total/float(tchars),total,tchars,pchars,lines);
    return 0;
}

int main_evalconf(int argc,char **argv) {
    if(argc!=2) throw "usage: ... dir";
    strbuf s;
    s.format("%s/[0-9][0-9][0-9][0-9]/[0-9][0-9][0-9][0-9].gt.txt",argv[1]);
    Glob files(s);
    float total = 0.0, tchars = 0, pchars = 0, lines = 0;
    intarray confusion(256,256); // FIXME
    confusion = 0;
    for(int index=0;index<files.length();index++) {
        if(index%1000==0)
            debugf("info","%s (%d/%d)\n",files(index),index,files.length());

        strbuf base;
        base = files(index);
        base.truncate(-7);

        char buf[100000];
        try {
            fgets(buf,sizeof buf,stdio(files(index),"r"));
        } catch(const char *error) {
            continue;
        }
        strbuf truth;
        truth = buf;

        strbuf s; s = base; s += ".txt";
        try {
            fgets(buf,sizeof buf,stdio(s.ptr(),"r"));
        } catch(const char *error) {
            continue;
        }
        strbuf predicted;
        predicted = buf;

        cleanup_for_eval(truth);
        cleanup_for_eval(predicted);
        nustring ntruth,npredicted;
        nustring_convert(ntruth,truth);
        nustring_convert(npredicted,predicted);
        float dist = edit_distance(confusion,ntruth,npredicted,1,1,1);

        total += dist;
        tchars += truth.length();
        pchars += predicted.length();
        lines++;

        debugf("transcript",
               "%g\t%s\t%s\t%s\n",
               dist,
               files(index),
               truth.ptr(),
               predicted.ptr());
    }
    intarray list(65536,3); // FIXME
    int row = 0;
    for(int i=0;i<confusion.dim(0);i++) {
        for(int j=0;j<confusion.dim(1);j++) {
            if(confusion(i,j)==0) continue;
            if(i==j) continue;
            list(row,0) = confusion(i,j);
            list(row,1) = i;
            list(row,2) = j;
            row++;
        }
    }
    intarray perm;
    rowsort(perm,list);
    for(int k=0;k<perm.length();k++) {
        int index = perm(k);
        int count = list(index,0);
        int i = list(index,1);
        int j = list(index,2);
        if(count==0) continue;
        printf("%6d   %3d %3d   %c %c\n",
            count,i,j,
            i==0?'_':(i>32&&i<128)?i:'?',
            j==0?'_':(j>32&&j<128)?j:'?');
    }
    return 0;
}

int main_findconf(int argc,char **argv) {
    if(argc!=4) throw "usage: ... dir from to";
    int from,to;
    if(sscanf(argv[2],"%d",&from)<1) {
        char c;
        sscanf(argv[2],"%c",&c);
        from = c;
    }
    if(sscanf(argv[3],"%d",&to)<1) {
        char c;
        sscanf(argv[3],"%c",&c);
        to = c;
    }
    strbuf s;
    s.format("%s/[0-9][0-9][0-9][0-9]/[0-9][0-9][0-9][0-9].gt.txt",argv[1]);
    Glob files(s);
    intarray confusion(256,256);
    for(int index=0;index<files.length();index++) {
        strbuf base;
        base = files(index);
        base.truncate(-7);
        char buf[100000];
        try {
            fgets(buf,sizeof buf,stdio(files(index),"r"));
        } catch(const char *error) {
            continue;
        }
        strbuf truth;
        truth = buf;

        strbuf s; s = base; s += ".txt";
        try {
            fgets(buf,sizeof buf,stdio(s.ptr(),"r"));
        } catch(const char *error) {
            continue;
        }
        strbuf predicted;
        predicted = buf;

        cleanup_for_eval(truth);
        cleanup_for_eval(predicted);
        nustring ntruth,npredicted;
        nustring_convert(ntruth,truth);
        nustring_convert(npredicted,predicted);
        confusion = 0;
        edit_distance(confusion,ntruth,npredicted,1,1,1);
        if(confusion(from,to)>0) {
            printf("%s.png\n",base.ptr());
        }
    }
    return 0;
}
int main_evalfiles(int argc,char **argv) {
    if(argc!=3) throw "usage: ... file1 file2";
    char buf[1000000];
    buf[0] = 0;
    fread(buf,1,sizeof buf,stdio(argv[1],"r"));
    strbuf truth;
    truth = buf;
    fread(buf,1,sizeof buf,stdio(argv[2],"r"));
    strbuf predicted;
    predicted = buf;

    cleanup_for_eval(truth);
    cleanup_for_eval(predicted);
    nustring ntruth,npredicted;
    nustring_convert(ntruth,truth);
    nustring_convert(npredicted,predicted);
    float dist = edit_distance(ntruth,npredicted);
    printf("dist %g tchars %d pchars %d\n",
           dist,truth.length(),predicted.length());
    return 0;
}

int main_buildhtml(int argc,char **argv) {
    throw Unimplemented();
}

int main_cleanhtml(int argc,char **argv) {
    throw Unimplemented();
}

void usage(const char *program) {
    fprintf(stderr,"usage:\n");
#define D(s,s2) {fprintf(stderr,"   %s %s\n",program,s); fprintf(stderr,"      %s\n",s2); }
    D("book2pages dir image image ...",
      "convert the input into a set of pages under dir/...");
    D("pages2lines dir",
      "convert the pages in dir/... into lines");
    D("lines2fsts dir",
      "convert the lines in dir/... into fsts (lattices)");
    D("fsts2bestpaths dir",
      "find the best interpretation of the fsts in dir/... without a language model");
    D("fsts2bestpaths langmod dir",
      "find the best interpretation of the fsts in dir/... with a language model");
    D("evaluate dir",
      "evaluate the quality of the OCR output in dir/...");
    D("evalconf dir",
      "evaluate the quality of the OCR output in dir/... and outputs confusion matrix");
    D("findconf dir from to",
      "finds instances of confusion of from to to (according to edit distance)");
    D("buildhtml dir",
      "creates an HTML representation of the OCR output in dir/...");
    D("cleanhtml dir",
      "removes all files from dir/... that aren't needed for the HTML output");
    exit(1);
}

int main(int argc,char **argv) {
    try {
        if(argc<2) usage(argv[0]);
        if(!strcmp(argv[1],"book2lines")) return main_pages2lines(argc-1,argv+1);
        if(!strcmp(argv[1],"book2pages")) return main_book2pages(argc-1,argv+1);
        if(!strcmp(argv[1],"pages2images")) return main_pages2images(argc-1,argv+1);
        if(!strcmp(argv[1],"pages2lines")) return main_pages2lines(argc-1,argv+1);
        if(!strcmp(argv[1],"evaluate")) return main_evaluate(argc-1,argv+1);
        if(!strcmp(argv[1],"evalconf")) return main_evalconf(argc-1,argv+1);
        if(!strcmp(argv[1],"findconf")) return main_findconf(argc-1,argv+1);
        if(!strcmp(argv[1],"evaluate1")) return main_evalfiles(argc-1,argv+1);
        if(!strcmp(argv[1],"buildhtml")) return main_buildhtml(argc-1,argv+1);
        if(!strcmp(argv[1],"cleanhtml")) return main_buildhtml(argc-1,argv+1);
        if(!strcmp(argv[1],"tesslines")) return main_tesslines(argc-1,argv+1);
        usage(argv[0]);
    } catch(const char *s) {
        fprintf(stderr,"FATAL: %s\n",s);
    }
}
