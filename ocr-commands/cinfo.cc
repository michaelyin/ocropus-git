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
    extern void linerec_load(autodel<IRecognizeLine> &,const char *);

#ifdef OBSOLETE_FILE_FORMATS
    int main_cinfo(int argc,char **argv) {
        autodel<IComponent> comp;
        stdio model(argv[1],"r");
        if(!model) {
            fprintf(stderr,"%s: could not open\n",argv[1]);
            return 1;
        }
        try {
            comp = load_component(model);
            comp->info();
        } catch(...) {
            fprintf(stderr,"%s: load failed as IComponent\n",argv[1]);
            try {
                autodel<IRecognizeLine> linerec;
                linerec_load(linerec,argv[1]);
                linerec->info();
            } catch(...) {
                fprintf(stderr,"%s: load failed as IRecognizeLine\n",argv[1]);
            }
        }
        return 0;
    }
#else
    int main_cinfo(int argc,char **argv) {
        autodel<IComponent> comp;
        stdio model(argv[1],"r");
        if(!model) {
            fprintf(stderr,"%s: could not open\n",argv[1]);
            return 1;
        }
        comp = load_component(model);
        comp->info();
        return 0;
    }
#endif
    int main_linfo(int argc,char **argv) {
        autodel<IRecognizeLine> linerec;
        stdio model(argv[1],"r");
        if(!model) {
            fprintf(stderr,"%s: could not open\n",argv[1]);
            return 1;
        }
        linerec_load(linerec,argv[1]);
        if(!linerec) {
            fprintf(stderr,"%s: load failed\n",argv[1]);
        } else {
            linerec->info();
        }
        return 0;
    }

    int main_params(int argc,char **argv) {
        if(argc<2) throwf("usage: %s classname\n",argv[0]);
        ocropus::global_verbose_params = "";
        autodel<IComponent> result;
        try {
            result = component_construct(argv[1]);
            printf("\n");
            printf("name=%s\n",result->name());
            printf("description=%s\n",result->description());
        } catch(const char *err) {
            fprintf(stderr,"%s: %s\n",argv[1],err);
        }
        return 0;
    }

    int main_components(int argc,char **argv) {
        narray<const char *> kinds;
        narray<const char *> names;
        list_components(names);
        for(int i=0;i<names.length();i++) {
            try {
                autodel<IComponent> p;
                p = component_construct(names[i]);
                const char *kind = p->interface();
                bool found = 0;
                for(int i=0;i<kinds.length();i++) {
                    if(!strcmp(kind,kinds[i])) {
                        found = 1;
                        break;
                    }
                }
                if(!found) kinds.push(kind);
            } catch(const char *err) {
                printf("FAILED to instantiate %-32s (%s)\n",names[i],err);
            } catch(const char *err) {
                printf("FAILED to instantiate %s: %s\n",names[i],err);
            }
        }
        for(int j=0;j<kinds.length();j++) {
            printf("\n");
            printf("################################################################\n");
            printf("### %s\n",kinds[j]);
            printf("################################################################\n");
            printf("\n");
            for(int i=0;i<names.length();i++) {
                try {
                    autodel<IComponent> p;
                    p = component_construct(names[i]);
                    if(strcmp(p->interface(),kinds[j])) continue;
                    strg desc(p->description());
                    int where = desc.find("\n");
                    if(where!=desc.npos) desc = desc.substr(0,where);
                    if(desc.length()>60) desc = desc.substr(0,60);
                    printf("%-32s %-32s\n    %s\n",names[i],p->name(),desc.c_str());
                } catch(const char *err) {
                    printf("FAILED to instantiate %-32s (%s)\n",names[i],err);
                } catch(const char *err) {
                    printf("FAILED to instantiate %s: %s\n",names[i],err);
                }
            }
        }
        return 0;
    }
}
