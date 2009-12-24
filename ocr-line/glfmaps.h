#ifndef glfmaps_h__
#define glfmaps_h__

#include "ocropus.h"
#include "glclass.h"

namespace glinerec {
    using namespace colib;
    using namespace ocropus;

    struct IFeatureMap : IComponent {
        const char *interface() { return "IFeatureMap"; }
        virtual void setLine(bytearray &image) = 0;
        virtual void extractFeatures(floatarray &v,
                                     rectangle b,
                                     bytearray &mask) = 0;
    };

    IFeatureMap *make_SimpleFeatureMap();
    void init_glfmaps();
}

#endif
