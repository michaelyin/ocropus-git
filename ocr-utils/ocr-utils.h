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
// File: ocr-utils.h
// Purpose: miscelaneous routines
// Responsible: mezhirov
// Reviewer:
// Primary Repository:
// Web Sites: www.iupr.org, www.dfki.de


#ifndef ocr_utils_h__
#define ocr_utils_h__

#include "ocropus.h"
#include "arraypaint.h"
#include "didegrade.h"
#include "docproc.h"
#include "editdist.h"
#include "enumerator.h"
#include "grid.h"
#include "grouper.h"
#include "linesegs.h"
#include "logger.h"
#include "pagesegs.h"
#include "queue.h"
#include "resource-path.h"
#include "segmentation.h"
#include "stringutil.h"
#include "sysutil.h"
#include "xml-entities.h"

namespace ocropus {
    void skeletal_features(bytearray &endpoints,
                           bytearray &junctions,
                           bytearray &image,
                           float presmooth=0.0,
                           float skelsmooth=0.0);
    void skeletal_feature_counts(int &nendpoints,
                                 int &njunctions,
                                 bytearray &image,
                                 float presmooth=0.0,
                                 float skelsmooth=0.0);
    int endpoints_counts(bytearray &image,float presmooth=0.0,float skelsmooth=0.0);
    int junction_counts(bytearray &image,float presmooth=0.0,float skelsmooth=0.0);
    int component_counts(bytearray &image,float presmooth=0.0);
    int hole_counts(bytearray &image,float presmooth=0.0);
    void extract_holes(bytearray &holes,bytearray &binarized);
    void compute_troughs(floatarray &troughs,bytearray &binarized,float rsmooth=1.0);
    void ridgemap(narray<floatarray> &maps,bytearray &binarized,
                  float rsmooth=1.0,float asigma=0.7,float mpower=0.5,
                  float rpsmooth=1.0);
}
#endif
