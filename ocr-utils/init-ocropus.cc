#include "ocropus.h"

namespace ocropus {
    void init_ocropus() {
        component_register("binarize-otsu",make_BinarizeByOtsu);
        component_register("binarize-range",make_BinarizeByRange);
        component_register("binarize-sauvola",make_BinarizeBySauvola);
        component_register("deskew-grayrast",make_DeskewGrayPageByRAST);
        component_register("deskew-rast",make_DeskewPageByRAST);
        component_register("docclean-generic",make_DocClean);
        component_register("docclean-pageframerast",make_PageFrameRAST);
        component_register("docclean-removeimage-binary",make_RemoveImageRegionsBinary);
        component_register("docclean-removeimage-gray",make_RemoveImageRegionsGray);
        component_register("recline-tess",make_TesseractRecognizeLine);
        component_register("seg-line-skel",make_SkelSegmenter);
        component_register("segline-ccs",make_ConnectedComponentSegmenter);
        component_register("segline-ccs",make_SegmentLineByCCS);
        component_register("segline-curved",make_CurvedCutSegmenter);
        component_register("segline-curvedcc",make_CurvedCutWithCcSegmenter);
        component_register("segline-proj",make_SegmentLineByProjection);
        component_register("segpage-1cp",make_SegmentPageBy1CP);
        component_register("segpage-morphtriv",make_SegmentPageByMorphTrivial);
        component_register("segpage-rast",make_SegmentPageByRAST);
        component_register("segpage-voronoi",make_SegmentPageByVORONOI);
        component_register("segpage-words",make_SegmentWords);
        component_register("segpage-xycuts",make_SegmentPageByXYCUTS);
        component_register("simple-grouper",make_SimpleGrouper);
        component_register("standard-grouper",make_StandardGrouper);
        component_register("textimage-leptonica",make_TextImageSegByLeptonica);
        component_register("textimage-logreg",make_TextImageSegByLogReg);
    }
}
