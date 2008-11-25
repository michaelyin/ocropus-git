// -*- C++ -*-

// Copyright 2006-2008 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz
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
// Project: OCRopus
// File: ocr-layout.h
// Purpose: Top level public header file for including layout analysis
//          functionality
// Responsible: Faisal Shafait (faisal.shafait@dfki.de)
// Reviewer:
// Primary Repository:
// Web Sites: www.iupr.org, www.dfki.de

#ifndef ocr_layout_h__
#define ocr_layout_h__

namespace ocropus {

    // Deskew page by RAST
    colib::ICleanupBinary *make_DeskewPageByRAST();

    // Text/Image segmentation
    colib::ICleanupBinary *make_RemoveImageRegionsBinary();
    colib::ICleanupGray   *make_RemoveImageRegionsGray();
    colib::ITextImageClassification *make_TextImageSegByLogReg();
    colib::ITextImageClassification *make_TextImageSegByLeptonica();

    // Page segmentation and layout analysis
    colib::ISegmentPage *make_SegmentPageByVORONOI();
    colib::ISegmentPage *make_SegmentPageByMorphTrivial();
    colib::ISegmentPage *make_SegmentPageBy1CP();
    colib::ISegmentPage *make_SegmentPageByRAST();
    colib::ISegmentPage *make_SegmentPageByVORONOI();
    //colib::ISegmentPage *make_SegmentPageByXYCUTS();
    colib::ISegmentPage *make_SegmentWords();

}
#endif
