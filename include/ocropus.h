#ifndef ocropus_headers_
#define ocropus_headers_

#include "colib/colib.h"
#include "iulib/iulib.h"

namespace ocropus {
    using namespace colib;
    using namespace iulib;

    ISegmentPage *make_SegmentPageByVORONOI();
    IBinarize *make_BinarizeByOtsu();
    IBinarize *make_BinarizeBySauvola();
    ICleanupBinary *make_DeskewPageByRAST();
    ICleanupBinary *make_RemoveImageRegionsBinary();
    ICleanupGray   *make_RemoveImageRegionsGray();
    ISegmentPage *make_SegmentPageByMorphTrivial();
    ISegmentPage *make_SegmentPageBy1CP();
    ISegmentPage *make_SegmentPageByRAST();
    ISegmentPage *make_SegmentPageByVORONOI();
/* ISegmentPage *make_SegmentPageByXYCUTS(); */
    ISegmentPage *make_SegmentWords();
    ISegmentLine *make_SegmentLineByProjection();
    ISegmentLine *make_SegmentLineByCCS();
    ISegmentLine *make_ConnectedComponentSegmenter();
    ISegmentLine *make_CurvedCutSegmenter();
    ISegmentLine *make_SkelSegmenter();
    IBinarize *make_BinarizeByRange();
    IBinarize *make_BinarizeByOtsu();
    IBinarize *make_BinarizeBySauvola();
    ITextImageClassification *make_TextImageSegByLogReg();
    ITextImageClassification *make_TextImageSegByLeptonica();
}

#include "line-info.h"
#include "lattice.h"

// these come from ocr-utils

#include "didegrade.h"
#include "editdist.h"
#include "enumerator.h"
#include "grid.h"
#include "grouper.h"
#include "logger.h"
#include "narray-io.h"
#include "segmentation.h"
#include "docproc.h"
#include "stringutil.h"
#include "arraypaint.h"
#include "pages.h"
#include "queue.h"
#include "pagesegs.h"
#include "linesegs.h"
#include "resource-path.h"
#include "segmentation.h"
#include "sysutil.h"
#include "xml-entities.h"

#ifndef CHECK
#define CHECK(X) CHECK_ARG(X)
#endif

#endif
