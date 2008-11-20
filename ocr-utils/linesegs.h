#ifndef linesegs_h__
#define linesegs_h__

#include "ocropus.h"

namespace ocropus {
    void check_line_segmentation(intarray &cseg);
    void make_line_segmentation_black(intarray &a);
    void make_line_segmentation_white(intarray &a);
}

#endif
