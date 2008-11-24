// -*- C++ -*-

// Copyright 2006 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz
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

#ifndef fstutil_h__
#define fstutil_h__

namespace ocropus {
    inline void check_valid_symbol(int symbol) {
        CHECK_ARG(symbol>0 && symbol<(1<<30));
    }

    struct Arcs {
        fst::StdVectorFst &fst;
        colib::autodel<fst::ArcIterator<fst::StdVectorFst> > arcs;
        int current_state;
        int current_arc;
        Arcs(fst::StdVectorFst &fst) : fst(fst) {
            current_state = -1;
            current_arc = -1;
        }
        int length() {
            return fst.NumStates();
        }
        int narcs(int i) {
            return fst.NumArcs(i);
        }
        const fst::StdArc &arc(int i,int j) {
            seek(i,j);
            return arcs->Value();
        }
    private:
        void seek(int i,int j) {
            if(!arcs || i!=current_state) {
                arcs = new fst::ArcIterator<fst::StdVectorFst>(fst,i);
                current_arc = 0;
            }
            if(j==current_arc) {
                // do nothing
            } if(current_arc<j+3) {
                while(current_arc<j) {
                    current_arc++;
                    arcs->Next();
                }
            } else {
                arcs->Seek(j);
                current_arc = j;
            }
        }
    };

};

#endif
