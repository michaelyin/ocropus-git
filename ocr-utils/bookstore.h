#ifndef bookstore_h__
#define bookstore_h__

#include "colib/colib.h"
#include "iulib/components.h"

namespace ocropus {
    struct IBookStore : IComponent {
        const char *interface() { return "IBookStore"; }

        virtual void setPrefix(const char *s) = 0;

        virtual bool getPage(bytearray &image,int page,const char *variant=0) = 0;
        virtual bool getPage(intarray &image,int page,const char *variant=0) = 0;
        virtual bool getLine(bytearray &image,int page,int line,const char *variant=0) = 0;
        virtual bool getLine(intarray &s,int page,int line,const char *variant=0) = 0;
        virtual bool getLine(ustrg &s,int page,int line,const char *variant=0) = 0;

        virtual void putPage(bytearray &image,int page,const char *variant=0) = 0;
        virtual void putPage(intarray &image,int page,const char *variant=0) = 0;
        virtual void putLine(bytearray &image,int page,int line,const char *variant=0) = 0;
        virtual void putLine(intarray &image,int page,int line,const char *variant=0) = 0;
        virtual void putLine(ustrg &s,int page,int line,const char *variant=0) = 0;

        virtual strg path(int page,int line=-1,const char *variant=0,const char *extension=0) = 0;
        virtual FILE *open(const char *mode,int page,int line=-1,const char *variant=0,const char *extension=0) = 0;

        virtual int numberOfPages() = 0;
        virtual int linesOnPage(int i) = 0;
        virtual int getLineId(int i,int j) = 0;

        void getLineBin(bytearray &image,int page,int line,const char *variant=0) {
            strg v = "bin";
            if(variant) { v += "."; v += variant; }
            getLine(image,page,line,variant);
        }
        void putLineBin(bytearray &image,int page,int line,const char *variant=0) {
            strg v = "bin";
            if(variant) { v += "."; v += variant; }
            putLine(image,page,line,variant);
        }

        void getPageSegmentation(intarray &image,int page,const char *variant=0) {
            strg v = "pseg";
            if(variant) { v += "."; v += variant; }
            getPage(image,page,variant);
            check_page_segmentation(image);
            make_page_segmentation_black(image);
        }
        void putPageSegmentation(intarray &image,int page,const char *variant=0) {
            strg v = "pseg";
            if(variant) { v += "."; v += variant; }
            check_page_segmentation(image);
            make_page_segmentation_white(image);
            putPage(image,page,variant);
            make_page_segmentation_black(image);
        }

        void getLineSegmentation(intarray &image,int page,int line,const char *variant=0) {
            strg v = "rseg";
            if(variant) { v += "."; v += variant; }
            getLine(image,page,line,variant);
            make_line_segmentation_black(image);
        }
        void putLineSegmentation(intarray &image,int page,int line,const char *variant=0) {
            strg v = "rseg";
            if(variant) { v += "."; v += variant; }
            make_line_segmentation_white(image);
            putLine(image,page,line,variant);
            make_line_segmentation_black(image);
        }

        void getCharSegmentation(intarray &image,int page,int line,const char *variant=0) {
            strg v = "cseg";
            if(variant) { v += "."; v += variant; }
            getLine(image,page,line,variant);
            make_line_segmentation_black(image);
        }
        void putCharSegmentation(intarray &image,int page,int line,const char *variant=0) {
            strg v = "cseg";
            if(variant) { v += "."; v += variant; }
            make_line_segmentation_white(image);
            putLine(image,page,line,variant);
            make_line_segmentation_black(image);
        }

        void getLattice(IGenericFst &fst,int page,int line,const char *variant=0) {
            strg s(path(page,line,variant,"fst"));
            fst.load(s);
        }
        void putLattice(IGenericFst &fst,int page,int line,const char *variant=0) {
            strg s(path(page,line,variant,"fst"));
            fst.save(s);
        }

    };

    IBookStore *make_OldBookStore();
    IBookStore *make_BookStore();
    IBookStore *make_SmartBookStore();
}

#endif
