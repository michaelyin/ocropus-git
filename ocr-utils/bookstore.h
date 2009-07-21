#ifndef bookstore_h__
#define bookstore_h__

#include "colib/colib.h"
#include "components.h"

namespace ocropus {
    struct IBookStore : IComponent {
        virtual void setPrefix(const char *s) = 0;

        virtual bool getPage(bytearray &image,int page,const char *variant=0) = 0;
        virtual bool getPage(intarray &image,int page,const char *variant=0) = 0;
        virtual bool getLine(bytearray &image,int page,int line,const char *variant=0) = 0;
        virtual bool getLine(intarray &s,int page,int line,const char *variant=0) = 0;
        virtual bool getLine(nustring &s,int page,int line,const char *variant=0) = 0;

        virtual void putPage(bytearray &image,int page,const char *variant=0) = 0;
        virtual void putPage(intarray &image,int page,const char *variant=0) = 0;
        virtual void putLine(bytearray &image,int page,int line,const char *variant=0) = 0;
        virtual void putLine(intarray &image,int page,int line,const char *variant=0) = 0;
        virtual void putLine(nustring &s,int page,int line,const char *variant=0) = 0;

        virtual iucstring path(int page,int line=-1,const char *variant=0,const char *extension=0) = 0;
        virtual FILE *open(const char *mode,int page,int line=-1,const char *variant=0,const char *extension=0) = 0;

        virtual int numberOfPages() = 0;
        virtual int linesOnPage(int i) = 0;
        virtual int getLineId(int i,int j) = 0;
    };

    IBookStore *make_OldBookStore();
    IBookStore *make_BookStore();
    IBookStore *make_SmartBookStore();
}

#endif
