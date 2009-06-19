#include <colib/colib.h>
#include <iulib/iulib.h>
#include "ocropus.h"
#include "bookstore.h"

using namespace colib;
using namespace iulib;
using namespace ocropus;

namespace {
}

namespace ocropus {
    struct OldBookStore : IBookStore {
        // FIXME make this OMP safe: allow parallel accesses etc.

        iucstring prefix;
        narray<intarray> lines;

        int get_max_page(const char *fpattern) {
            iucstring pattern;
            sprintf(pattern,"%s/%s",(const char *)prefix,fpattern);
            debugf("bookstore","pattern: %s\n",pattern.c_str());
            int npages = -1;
            Glob glob(pattern);
            for(int i=0;i<glob.length();i++) {
                int p = -1;
                int c = iucstring(glob(i)).rfind("/");
                CHECK(c>=0);
                CHECK(sscanf(glob(i)+c+1,"%d.png",&p)==1);
                if(p>npages) npages = p;
                debugf("bookstore","%4d %s [%d]\n",i,glob(i),npages);
            }
            npages++;
            return npages;
        }

        void get_lines_of_page(intarray &lines,int i) {
            iucstring pattern;
            sprintf(pattern,"%s/%04d/[0-9][0-9][0-9][0-9].png",(const char *)prefix,i);
            lines.clear();
            Glob glob(pattern);
            for(int i=0;i<glob.length();i++) {
                int k = -1;
                int c = iucstring(glob(i)).rfind("/");
                CHECK(c>=0);
                sscanf(glob(i)+c+1,"%d.png",&k);
                CHECK_ARG(k>=0 && k<=9999);
                lines.push(k);
            }
        }

        int linesOnPage(int i) {
            return lines(i).length();
        }

        int getLineId(int i,int j) {
            return lines(i)(j);
        }

        void setPrefix(const char *s) {
#pragma omp critical
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

        iucstring path(int page,int line=-1,const char *variant=0,const char *extension=0) {
            iucstring file;
            sprintf(file,"%s/%04d",(const char *)prefix,page);
            if(line>=0) sprintf_append(file,"/%04d",line);
            if(variant) sprintf_append(file,".%s",variant);
            if(extension) sprintf_append(file,".%s",extension);
            debugf("bookstore","(path: %s)\n",file.c_str());
            return file;
        }

        FILE *open(const char *mode,int page,int line,const char *variant=0,const char *extension=0) {
            iucstring s = path(page,line,variant,extension);
            return fopen(s,mode);
        }

        bool getPage(bytearray &image,int page,const char *variant=0) {
            try {
                read_image_gray(image,path(page,-1,variant,"png").c_str());
            } catch(...) { // FIXME do better checking here
                return false;
            }
            return true;
        }

        void putPage(bytearray &image,int page,const char *variant=0) {
            write_image_gray(path(page,-1,variant,"png").c_str(),image);
        }

        void putPage(intarray &image,int page,const char *variant=0) {
            write_image_packed(path(page,-1,variant,"png"),image);
        }

        bool getLine(bytearray &image,int page,int line,const char *variant=0) {
            try {
                read_image_gray(image,path(page,line,variant,"png").c_str());
            } catch(...) { // FIXME do better checking here
                return false;
            }
            return true;
        }

        void maybeMakeDirectory(int page) {
            iucstring s;
            sprintf(s,"%s/%04d",(const char *)prefix,page);
            mkdir(s,0777);
        }

        void putLine(bytearray &image,int page,int line,const char *variant=0) {
            maybeMakeDirectory(page);
            stdio stream(open("w",page,line,variant,"png"));
            write_image_gray(stream,image,"png");
        }

        bool getLine(iucstring &s,int page,int line,const char *variant=0) {
            stdio stream(open("r",page,line,variant,"txt"),true);
            if(!stream) return false;
            fread(s,stream);
            return true;
        }

        void putLine(iucstring &s,int page,int line,const char *variant=0) {
            maybeMakeDirectory(page);
            stdio stream(open("w",page,line,variant,"txt"));
            fwrite(s,stream);
        }

        int numberOfPages() {
            return lines.length();
        }

    };

    IBookStore *make_OldBookStore() {
        return new OldBookStore();
    }
}