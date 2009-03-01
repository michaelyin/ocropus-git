#include "colib/colib.h"
#include "iulib/iulib.h"
#include "components.h"

using namespace colib;

namespace ocropus {

    ////////////////////////////////////////////////////////////////
    // constructing, loading, and saving classifiers
    ////////////////////////////////////////////////////////////////

    typedef IComponent *(*component_constructor)();

    strhash<component_constructor> constructors;

    void component_register_fun(const char *name,component_constructor f,bool replace) {
        if(constructors.find(name) && !replace)
            throwf("%s: already registered as a component",name);
        constructors(name) = f;
    }

    component_constructor component_lookup(const char *name) {
        if(!constructors.find(name)) throwf("%s: no such component",name);
        return constructors(name);
    }

    IComponent *component_construct(const char *name) {
        component_constructor f = component_lookup(name);
        return f();
    }

    static int level = 0;

    void save_component(FILE *stream,IComponent *classifier) {
        if(!classifier) {
            debugf("iodetail","%*s[writing OBJ:NULL]\n",level,"");
            debugf("iodetail","OBJ:NULL",stream);
        } else {
            debugf("iodetail","%*s[writing OBJ:%s]\n",level,"",classifier->name());
            level++;
            fputs("OBJ:",stream);
            fputs(classifier->name(),stream);
            fputs("\n",stream);
            classifier->save(stream);
            fputs("OBJ:END\n",stream);
            level--;
            debugf("iodetail","%*s[done]\n",level,"");
        }
    }

    IComponent *load_component(FILE *stream) {
        char buf[1000];
        fgets(buf,sizeof buf,stream);
        if(strlen(buf)>0) buf[strlen(buf)-1] = 0;
        debugf("iodetail","%*s[got %s]\n",level,"",buf);
        IComponent *result = 0;
        if(strcmp(buf,"OBJ:NULL")) {
            level++;
            CHECK(!strncmp(buf,"OBJ:",4));
            result = component_construct(buf+4);
            result->load(stream);
            fgets(buf,sizeof buf,stream);
            if(strlen(buf)>0) buf[strlen(buf)-1] = 0;
            ASSERT(!strcmp(buf,"OBJ:END"));
            level--;
        }
        debugf("iodetail","%*s[done]\n",level,"");
        return result;
    }
}

