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
    extern void cleanup_for_eval(strg &);

    int main_evaluate(int argc,char **argv) {
        if(argc!=2) throw "usage: ... dir";
        strg s;
        sprintf(s, "%s/[0-9][0-9][0-9][0-9]/[0-9][0-9][0-9][0-9].gt.txt",argv[1]);
        Glob files(s);
        float total = 0.0, tchars = 0, pchars = 0, lines = 0;
        for(int index=0;index<files.length();index++) {
            if(index%1000==0)
                debugf("info","%s (%d/%d)\n",files(index),index,files.length());

            strg base = files(index);
            base.erase(base.find("."));

            strg truth;
            try {
                fgets(truth, stdio(files(index),"r"));
            } catch(const char *error) {
                continue;
            }

            strg s = base + ".txt";
            strg predicted;
            try {
                fgets(predicted, stdio(s,"r"));
            } catch(const char *error) {
                continue;
            }

            cleanup_for_eval(truth);
            cleanup_for_eval(predicted);
            ustrg ntruth = truth;
            ustrg npredicted = predicted;
            float dist = edit_distance(ntruth,npredicted);

            total += dist;
            tchars += truth.length();
            pchars += predicted.length();
            lines++;

            debugf("transcript",
                    "%g\t%s\t%s\t%s\n",
                    dist,
                    files(index),
                    truth.c_str(),
                    predicted.c_str());
        }
        printf("rate %g total_error %g true_chars %g predicted_chars %g lines %g\n",
                total/float(tchars),total,tchars,pchars,lines);
        return 0;
    }

    int main_evalconf(int argc,char **argv) {
        if(argc!=2) throw "usage: ... dir";
        strg s;
        sprintf(s,"%s/[0-9][0-9][0-9][0-9]/[0-9][0-9][0-9][0-9].gt.txt",argv[1]);
        Glob files(s);
        float total = 0.0, tchars = 0, pchars = 0, lines = 0;
        intarray confusion(256,256); // FIXME/tmb limited to 256x256, replace with int2hash
        confusion = 0;
        for(int index=0;index<files.length();index++) {
            if(index%1000==0)
                debugf("info","%s (%d/%d)\n",files(index),index,files.length());

            strg base = files(index);
            base.erase(base.length()-7);

            strg truth;
            try {
                fgets(truth, stdio(files(index),"r"));
            } catch(const char *error) {
                continue;
            }

            strg s = base + ".txt";
            strg predicted;
            try {
                fgets(predicted, stdio(s,"r"));
            } catch(const char *error) {
                continue;
            }

            cleanup_for_eval(truth);
            cleanup_for_eval(predicted);
            ustrg ntruth = truth;
            ustrg npredicted = predicted;
            float dist = edit_distance(confusion,ntruth,npredicted,1,1,1);

            total += dist;
            tchars += truth.length();
            pchars += predicted.length();
            lines++;

            debugf("transcript",
                    "%g\t%s\t%s\t%s\n",
                    dist,
                    files(index),
                    truth.c_str(),
                    predicted.c_str());
        }
        intarray list(65536,3); // FIXME/tmb replace with hash table when we move to Unicode
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
        strg s;
        sprintf(s,"%s/[0-9][0-9][0-9][0-9]/[0-9][0-9][0-9][0-9].gt.txt",argv[1]);
        Glob files(s);
        intarray confusion(256,256);
        for(int index=0;index<files.length();index++) {
            strg base = files(index);
            base.erase(base.length()-7);


            strg truth;
            try {
                fgets(truth, stdio(files(index),"r"));
            } catch(const char *error) {
                continue;
            }

            strg s = base + ".txt";
            strg predicted;
            try {
                fgets(predicted, stdio(s,"r"));
            } catch(const char *error) {
                continue;
            }

            cleanup_for_eval(truth);
            cleanup_for_eval(predicted);
            ustrg ntruth = truth;
            ustrg npredicted = predicted;
            confusion = 0;
            edit_distance(confusion,ntruth,npredicted,1,1,1);
            if(confusion(from,to)>0) {
                printf("%s.png\n",base.c_str());
            }
        }
        return 0;
    }
    int main_evalfiles(int argc,char **argv) {
        if(argc!=3) throw "usage: ... file1 file2";
        strg truth;
        fread(truth, stdio(argv[1],"r"));
        strg predicted;
        fread(predicted, stdio(argv[2],"r"));

        cleanup_for_eval(truth);
        cleanup_for_eval(predicted);
        ustrg ntruth = truth;
        ustrg npredicted = predicted;

        float dist = edit_distance(ntruth,npredicted);
        printf("dist %g tchars %d pchars %d\n",
                dist,truth.length(),predicted.length());
        return 0;
    }
}
