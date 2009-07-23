#include <colib/colib.h>
#include <iulib/iulib.h>
#include "ocropus.h"
#include "bookstore.h"

using namespace colib;
using namespace iulib;
using namespace ocropus;

namespace ocropus {
    struct OldBookStore : IBookStore {
        // FIXME make this OMP safe: allow parallel accesses etc.

        strg prefix;
        narray<intarray> lines;

        virtual int get_max_page(const char *fpattern) {
            int npages = -1;
            {
                strg pattern;
                sprintf(pattern,"%s/%s",(const char *)prefix,fpattern);
                debugf("bookstore","pattern: %s\n",pattern.c_str());
                Glob glob(pattern);
                for(int i=0;i<glob.length();i++) {
                    int p = -1;
                    int c = strg(glob(i)).rfind("/");
                    CHECK(c>=0);
                    CHECK(sscanf(glob(i)+c+1,"%d.png",&p)==1);
                    if(p>npages) npages = p;
                    debugf("bookstore","%4d %s [%d]\n",i,glob(i),npages);
                }
                npages++;
            }
            return npages;
        }

        virtual void get_lines_of_page(intarray &lines,int i) {
            {
                strg pattern;
                sprintf(pattern,"%s/%04d/[0-9][0-9][0-9][0-9].png",(const char *)prefix,i);
                debugf("bookstore","pattern: %s\n",pattern.c_str());

                lines.clear();
                Glob glob(pattern);
                for(int i=0;i<glob.length();i++) {
                    int k = -1;
                    int c = strg(glob(i)).rfind("/");
                    CHECK(c>=0);
                    sscanf(glob(i)+c+1,"%d.png",&k);
                    CHECK_ARG(k>=0 && k<=9999);
                    lines.push(k);
                    debugf("bookstore","adding: %s\n",glob(i));
                }
            }
        }

        int linesOnPage(int i) {
            int result;
            result = lines(i).length();
            return result;
        }

        int getLineId(int i,int j) {
            int result;
            result = lines(i)(j);
            return result;
        }

        void setPrefix(const char *s) {
            {
                prefix = s;
                int ndirs = get_max_page("[0-9][0-9][0-9][0-9]");
                CHECK(ndirs<10000);
                int npngs = get_max_page("[0-9][0-9][0-9][0-9].png");
                CHECK(npngs<10000);
                int npages = max(ndirs,npngs);
                CHECK(npages<10000);
                lines.resize(npages);
                for(int i=0;i<npages;i++) {
                    get_lines_of_page(lines(i),i);
                    debugf("bookstore","page %d #lines %d\n",i,lines(i).length());
                }
            }
        }

        virtual strg path(int page,int line=-1,const char *variant=0,const char *extension=0) {
            strg file;
            {
                sprintf(file,"%s/%04d",(const char *)prefix,page);
                if(line>=0) sprintf_append(file,"/%04d",line);
                if(variant) sprintf_append(file,".%s",variant);
                if(extension) sprintf_append(file,".%s",extension);
                debugf("bookstore","(path: %s)\n",file.c_str());
            }
            return file;
        }

        FILE *open(const char *mode,int page,int line,const char *variant=0,const char *extension=0) {
            {
                strg s = path(page,line,variant,extension);
                return fopen(s,mode);
            }
        }

        bool getPage(bytearray &image,int page,const char *variant=0) {
            strg s = path(page,-1,variant,"png");
            if(!file_exists(s)) return false;
            read_image_gray(image,s);
            return true;
        }

        bool getPage(intarray &image,int page,const char *variant=0) {
            strg s = path(page,-1,variant,"png");
            if(!file_exists(s)) return false;
            read_image_packed(image,s);
            return true;
        }

        void putPage(bytearray &image,int page,const char *variant=0) {
            write_image_gray(path(page,-1,variant,"png").c_str(),image);
        }

        void putPage(intarray &image,int page,const char *variant=0) {
            write_image_packed(path(page,-1,variant,"png"),image);
        }

        bool getLine(bytearray &image,int page,int line,const char *variant=0) {
            strg s = path(page,line,variant,"png");
            if(!file_exists(s)) return false;
            read_image_gray(image,s);
            return true;
        }

        bool getLine(intarray &image,int page,int line,const char *variant=0) {
            strg s = path(page,line,variant,"png");
            if(!file_exists(s)) return false;
            read_image_packed(image,s);
            return true;
        }

        void maybeMakeDirectory(int page) {
            strg s;
            sprintf(s,"%s/%04d",(const char *)prefix,page);
            mkdir(s,0777);
        }

        void putLine(bytearray &image,int page,int line,const char *variant=0) {
            maybeMakeDirectory(page);
            stdio stream(open("w",page,line,variant,"png"));
            write_image_gray(stream,image,"png");
        }

        void putLine(intarray &image,int page,int line,const char *variant=0) {
            maybeMakeDirectory(page);
            stdio stream(open("w",page,line,variant,"png"));
            write_image_packed(stream,image,"png");
        }

        bool getLine(ustrg &str,int page,int line,const char *variant=0) {
            stdio stream(open("r",page,line,variant,"txt"),true);
            if(!stream) return false;
            utf8strg utf8;
            utf8.fread(stream);
            str.utf8Decode(utf8);
            return true;
        }

        void putLine(ustrg &str,int page,int line,const char *variant=0) {
            utf8strg utf8;
            str.utf8Encode(utf8);
            maybeMakeDirectory(page);
            stdio stream(open("w",page,line,variant,"txt"));
            utf8.fwrite(stream);
        }

        int numberOfPages() {
            return lines.length();
        }

    };

    struct BookStore : OldBookStore {
        virtual void get_lines_of_page(intarray &lines,int i) {
            {
                strg pattern;
                sprintf(pattern,"%s/%04d/[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F].png",(const char *)prefix,i);
                debugf("bookstore","pattern: %s\n",pattern.c_str());
                lines.clear();
                Glob glob(pattern);
                for(int i=0;i<glob.length();i++) {
                    int k = -1;
                    int c = strg(glob(i)).rfind("/");
                    CHECK(c>=0);
                    sscanf(glob(i)+c+1,"%x.png",&k);
                    lines.push(k);
                }
            }
        }
        virtual strg path(int page,int line=-1,const char *variant=0,const char *extension=0) {
            strg file;
            {
                sprintf(file,"%s/%04d",(const char *)prefix,page);
                if(line>=0) sprintf_append(file,"/%06x",line);
                if(variant) sprintf_append(file,".%s",variant);
                if(extension) sprintf_append(file,".%s",extension);
                debugf("bookstore","(path: %s)\n",file.c_str());
            }
            return file;
        }

    };

    struct SmartBookStore : IBookStore {
        autodel<IBookStore> p;

        virtual void setPrefix(const char *prefix) {
            {
                strg pattern;
                sprintf(pattern,"%s/[0-9][0-9][0-9][0-9]/[0-9][0-9][0-9][0-9].png",(const char *)prefix);
                debugf("debug","checking %s\n",pattern.c_str());
                Glob glob(pattern);
                if(glob.length()>0) {
                    debugf("info","selecting OldBookStore\n");
                    p = new OldBookStore();
                } else {
                    debugf("info","selecting (new) BookStore\n");
                    p = new BookStore();
                }
                p->setPrefix(prefix);
            }
        }

        virtual bool getPage(bytearray &image,int page,const char *variant=0) { return p->getPage(image,page,variant); }
        virtual bool getPage(intarray &image,int page,const char *variant=0) { return p->getPage(image,page,variant); }
        virtual bool getLine(bytearray &image,int page,int line,const char *variant=0) { return p->getLine(image,page,line,variant); }
        virtual bool getLine(intarray &s,int page,int line,const char *variant=0) { return p->getLine(s,page,line,variant); }
        virtual bool getLine(ustrg &s,int page,int line,const char *variant=0) { return p->getLine(s,page,line,variant); }

        virtual void putPage(bytearray &image,int page,const char *variant=0) { p->putPage(image,page,variant); }
        virtual void putPage(intarray &image,int page,const char *variant=0) { p->putPage(image,page,variant); }
        virtual void putLine(bytearray &image,int page,int line,const char *variant=0) { p->putLine(image,page,line,variant); }
        virtual void putLine(intarray &image,int page,int line,const char *variant=0) { p->putLine(image,page,line,variant); }
        virtual void putLine(ustrg &s,int page,int line,const char *variant=0) { p->putLine(s,page,line,variant); }

        virtual strg path(int page,int line=-1,const char *variant=0,const char *extension=0) { return p->path(page,line,variant,extension); }
        virtual FILE *open(const char *mode,int page,int line=-1,const char *variant=0,const char *extension=0) { return p->open(mode,page,line,variant,extension); }

        virtual int numberOfPages() { return p->numberOfPages(); }
        virtual int linesOnPage(int i) { return p->linesOnPage(i); }
        virtual int getLineId(int i,int j) { return p->getLineId(i,j); }
    };

    IBookStore *make_OldBookStore() {
        return new OldBookStore();
    }

    IBookStore *make_BookStore() {
        return new BookStore();
    }

    IBookStore *make_SmartBookStore() {
        return new SmartBookStore();
    }
}

