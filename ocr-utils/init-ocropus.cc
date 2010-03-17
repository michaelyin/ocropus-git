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
        component_register("DpSegmenter",make_DpSegmenter);
        // component_register("SimpleFeatureMap",make_SimpleFeatureMap);
        // component_register("RidgeFeatureMap",make_RidgeFeatureMap);
        component_register("SimpleGrouper",make_SimpleGrouper);
        component_register("simplegrouper",make_SimpleGrouper);
        component_register("StandardGrouper",make_StandardGrouper);
        component_register("DocClean",make_DocClean);
        // component_register("DocCleanConComp",make_DocCleanConComp);
        component_register("PageFrameRAST",make_PageFrameRAST);
        component_register("DeskewPageByRAST",make_DeskewPageByRAST);
        component_register("DeskewGrayPageByRAST",make_DeskewGrayPageByRAST);
        component_register("TextImageSegByLogReg",make_TextImageSegByLogReg);
#ifdef HAVE_LEPTONICA
        component_register("RemoveImageRegionsBinary",make_RemoveImageRegionsBinary);
        component_register("RemoveImageRegionsGray",make_RemoveImageRegionsGray);
        component_register("TextImageSegByLeptonica",make_TextImageSegByLeptonica);
#endif
        component_register("SegmentPageByMorphTrivial",make_SegmentPageByMorphTrivial);
        component_register("SegmentPageBy1CP",make_SegmentPageBy1CP);
        component_register("SegmentPageByRAST",make_SegmentPageByRAST);
        component_register("SegmentPageByRAST1",make_SegmentPageByRAST1);
        component_register("SegmentPageByVORONOI",make_SegmentPageByVORONOI);
        component_register("SegmentPageByXYCUTS",make_SegmentPageByXYCUTS);
        component_register("SegmentWords",make_SegmentWords);
        component_register("OcroFST",make_OcroFST);
        component_register("BinarizeByRange",make_BinarizeByRange);
        component_register("BinarizeByOtsu",make_BinarizeByOtsu);
        component_register("BinarizeBySauvola",make_BinarizeBySauvola);
        component_register("BinarizeByHT",make_BinarizeByHT);
        component_register("SegmentLineByProjection",make_SegmentLineByProjection);
        component_register("SegmentLineByCCS",make_SegmentLineByCCS);
        component_register("SegmentLineByGCCS",make_SegmentLineByGCCS);
        component_register("ConnectedComponentSegmenter",make_ConnectedComponentSegmenter);
        component_register("CurvedCutSegmenter",make_CurvedCutSegmenter);
        component_register("CurvedCutWithCcSegmenter",make_CurvedCutWithCcSegmenter);
        component_register("SkelSegmenter",make_SkelSegmenter);
        component_register("OldBookStore",make_OldBookStore);
        component_register("BookStore",make_BookStore);
        component_register("SmartBookStore",make_SmartBookStore);
        component_register("Degradation",make_Degradation);
        component_register<Pages>("Pages");
        extern ICleanupBinary *make_RmHalftone();
        component_register("RmHalftone",make_RmHalftone);
        extern ICleanupBinary *make_RmUnderline();
        component_register("RmUnderline",make_RmUnderline);
        extern ICleanupBinary *make_RmBig();
        component_register("RmBig",make_RmBig);
        extern ICleanupBinary *make_AutoInvert();
        component_register("AutoInvert",make_AutoInvert);
        IBinarize *make_StandardPreprocessing();
        component_register("StandardPreprocessing",make_StandardPreprocessing);
    }
}
