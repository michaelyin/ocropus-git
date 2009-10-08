#ifndef ocr_commands_h__
#define ocr_commands_h__

#define CATCH_COMMON(ACTION) \
catch(const char *s) { \
    debugf("error","%s [%s]\n",s,exception_context); ACTION; \
} catch(Unimplemented &err) { \
    debugf("error","Unimplemented [%s]\n",exception_context); ACTION; \
} catch(BadTextLine &err) { \
    debugf("error","BadTextLine [%s]\n",exception_context); ACTION; \
} catch(...) { \
    debugf("error","%s\n",exception_context); ACTION;    \
} \
exception_context = ""

#ifndef DEFAULT_DATA_DIR
#define DEFAULT_DATA_DIR "/usr/local/share/ocropus/models/"
#endif
#ifndef DEFAULT_EXT_DIR
#define DEFAULT_EXT_DIR "/usr/local/share/ocropus/extensions/"
#endif

namespace ocropus {
    extern const char *exception_context;
    void cleanup_for_eval(strg &s);
    void cleanup_for_eval(ustrg &s);
    void linerec_load(autodel<IRecognizeLine> &linerec,const char *cmodel);
    void ustrg_convert(strg &output,ustrg &str);
    void ustrg_convert(ustrg &output,strg &str);
    void read_transcript(IGenericFst &fst, const char *path);
    void read_gt(IGenericFst &fst, const char *base);
    void scale_fst(OcroFST &fst,float scale);
    void store_costs(const char *base, floatarray &costs);
    void rseg_to_cseg(intarray &cseg, intarray &rseg, intarray &ids);
}

namespace glinerec {
    extern const char *command;
    IRecognizeLine *make_Linerec();
}

#define DLOPEN

#ifdef DLOPEN
#include <dlfcn.h>
#endif

#endif
