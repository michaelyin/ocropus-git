// Copyright 2007 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz
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
// File: resource-path.cc
// Purpose:
// Responsible: kofler
// Reviewer:
// Primary Repository:
// Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org

#include "resource-path.h"

static const iucstring default_path = "/usr/local/share/ocropus:"
                                  "/usr/share/ocropus";

using namespace colib;


static iucstring original_path;
static narray<iucstring> path;
static iucstring errmsg;


namespace ocropus {

    void set_resource_path(const char *s) {
        const char *env = getenv("OCROPUS_DATA");
        iucstring p;
        if(!s && !env)
            p = default_path;
        else if(!s)
            p = env;
        else if(!env)
            p = s;
        else {
            p = s;
            p += ":";
            p += env;
        }
        original_path = p;
        split_string(path, p, ":;");
    }

    FILE *open_resource(const char *relative_path) {
        if(!original_path)
            set_resource_path(NULL);
        for(int i = 0; i < path.length(); i++) {
            iucstring s = path[i] + "/" + relative_path;
            FILE *f = fopen(s, "rb");
            if(f)
                return f;
        }
        sprintf(errmsg, "Unable to find resource %s in the data path, which is %s", relative_path, original_path.c_str());
        fprintf(stderr, "%s\n", errmsg.c_str());
        fprintf(stderr, "Please check that your $OCROPUS_DATA variable points to the OCRopus data directory");
        throw errmsg.c_str();
    }

    void find_and_load_ICharacterClassifier(ICharacterClassifier &i,
                                            const char *resource) {
        i.load(stdio(open_resource(resource)));
    }
    void find_and_load_IRecognizeLine(IRecognizeLine &i,
                                      const char *resource) {
        i.load(stdio(open_resource(resource)));
    }
}
