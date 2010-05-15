// Copyright 2006 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz
// or its licensors, as applicable.
// Copyright 1995-2005 by Thomas M. Breuel
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
// Project: iulib -- image understanding library
// File:
// Purpose:
// Responsible:
// Reviewer:
// Primary Repository:
// Web Sites: www.iupr.org, www.dfki.de

#include "ocropus.h"
#include "glcuts.h"
#include "glfmaps.h"
#include "bookstore.h"

using namespace iulib;
using namespace ocropus;
using namespace glinerec;

namespace ocropus {
    void init_ocropus_components() {
        static bool init = false;
        if(init) return;
        init = true;
        component_register("DpSegmenter",make_DpSegmenter,true);
        // component_register("SimpleFeatureMap",make_SimpleFeatureMap,true);
        // component_register("RidgeFeatureMap",make_RidgeFeatureMap,true);
        component_register("SimpleGrouper",make_SimpleGrouper,true);
        component_register("simplegrouper",make_SimpleGrouper,true);
        component_register("StandardGrouper",make_StandardGrouper,true);
        component_register("DocClean",make_DocClean,true);
        // component_register("DocCleanConComp",make_DocCleanConComp,true);
        component_register("PageFrameRAST",make_PageFrameRAST,true);
        component_register("DeskewPageByRAST",make_DeskewPageByRAST,true);
        component_register("DeskewGrayPageByRAST",make_DeskewGrayPageByRAST,true);
        component_register("TextImageSegByLogReg",make_TextImageSegByLogReg,true);
#ifdef HAVE_LEPTONICA
        component_register("RemoveImageRegionsBinary",make_RemoveImageRegionsBinary,true);
        component_register("RemoveImageRegionsGray",make_RemoveImageRegionsGray,true);
        component_register("TextImageSegByLeptonica",make_TextImageSegByLeptonica,true);
#endif
        component_register("SegmentPageByMorphTrivial",make_SegmentPageByMorphTrivial,true);
        component_register("SegmentPageBy1CP",make_SegmentPageBy1CP,true);
        component_register("SegmentPageByRAST",make_SegmentPageByRAST,true);
        component_register("SegmentPageByRAST1",make_SegmentPageByRAST1,true);
        component_register("SegmentPageByVORONOI",make_SegmentPageByVORONOI,true);
        component_register("SegmentPageByXYCUTS",make_SegmentPageByXYCUTS,true);
        component_register("SegmentWords",make_SegmentWords,true);
        component_register("OcroFST",make_OcroFST,true);
        component_register("BinarizeByRange",make_BinarizeByRange,true);
        component_register("BinarizeByOtsu",make_BinarizeByOtsu,true);
        component_register("BinarizeBySauvola",make_BinarizeBySauvola,true);
        component_register("BinarizeByHT",make_BinarizeByHT,true);
        component_register("SegmentLineByProjection",make_SegmentLineByProjection,true);
        component_register("SegmentLineByCCS",make_SegmentLineByCCS,true);
        component_register("SegmentLineByGCCS",make_SegmentLineByGCCS,true);
        component_register("ConnectedComponentSegmenter",make_ConnectedComponentSegmenter,true);
        component_register("CurvedCutSegmenter",make_CurvedCutSegmenter,true);
        component_register("CurvedCutWithCcSegmenter",make_CurvedCutWithCcSegmenter,true);
        component_register("SkelSegmenter",make_SkelSegmenter,true);
        component_register("OldBookStore",make_OldBookStore,true);
        component_register("BookStore",make_BookStore,true);
        component_register("SmartBookStore",make_SmartBookStore,true);
        component_register("Degradation",make_Degradation,true);
        component_register<Pages>("Pages");
        extern ICleanupBinary *make_RmHalftone();
        component_register("RmHalftone",make_RmHalftone,true);
        extern ICleanupBinary *make_RmUnderline();
        component_register("RmUnderline",make_RmUnderline,true);
        extern ICleanupBinary *make_RmBig();
        component_register("RmBig",make_RmBig,true);
        extern ICleanupBinary *make_AutoInvert();
        component_register("AutoInvert",make_AutoInvert,true);
        IBinarize *make_StandardPreprocessing();
        component_register("StandardPreprocessing",make_StandardPreprocessing,true);
    }
}
