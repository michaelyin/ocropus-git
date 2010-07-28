#ifndef glcuts_h__
#define glcuts_h__

#include "ocropus.h"

namespace glinerec {
    using namespace ocropus;
    struct IDpSegmenter : ISegmentLine {
        float down_cost;
        float outside_diagonal_cost;
        float outside_diagonal_cost_r;
        float inside_diagonal_cost;
        float boundary_diagonal_cost;
        float inside_weight;
        float boundary_weight;
        float outside_weight;
        int min_range;
        float cost_smooth;
        float min_thresh;
        intarray dimage;
    };
    IDpSegmenter *make_DpSegmenter();
}

#endif
