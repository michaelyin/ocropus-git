#ifndef stringutil_h_
#define stringutil_h_

namespace ocropus {

    const char *get_version_string();
    void set_version_string(const char *);

    void throw_fmt(const char *format, ...);

    void strbuf_format(strbuf &str, const char *format, ...);
    void code_to_strbuf(strbuf &s, int code);

}

#endif
