#include <colib/colib.h>
#include <iulib/iulib.h>
#include <ocropus/ocropus.h>

using namespace colib;
using namespace iulib;
using namespace ocropus;

namespace ocropus { int main_ocropus(int,char **); }

struct MyThresholder : IBinarize {
    const char *name() { return "mythresholder"; }
    const char *description() { return "performs thresholding based on the mean"; }
    MyThresholder() {
        pdef("factor",1.0,"threshold is factor * mean");
    }
    void binarize(bytearray &out,floatarray &in) {
        float factor = pgetf("factor");
        float mean = sum(in)/in.length();
        debugf("info","threshold=%g\n",mean);
        int n = in.length();
        out.makelike(in);
        for(int i=0;i<n;i++)
            out[i] = 255 * (in[i]>=factor*mean);
    }
};

extern "C" {
    void ocropus_init_dl();
}

void ocropus_init_dl() {
    component_register<MyThresholder>("MyThresholder");
}

int main(int argc,char **argv) {
    component_register<MyThresholder>("MyThresholder");
    main_ocropus(argc,argv);
}
