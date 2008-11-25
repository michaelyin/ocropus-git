#ifndef linesegs_h__
#define linesegs_h__

#include "ocropus.h"

namespace ocropus {
    void check_line_segmentation(intarray &cseg);
    void make_line_segmentation_black(intarray &a);
    void make_line_segmentation_white(intarray &a);
    inline void write_line_segmentation(FILE *stream,intarray &a) {
        check_line_segmentation(a);
        make_line_segmentation_white(a);
        write_image_packed(stream,a,"png");
    }
    inline void read_line_segmentation(intarray &a,FILE *stream) {
        read_image_packed(a,stream,"png");
        check_line_segmentation(a);
        make_line_segmentation_black(a);
    }
}

#endif
