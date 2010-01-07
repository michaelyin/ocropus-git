#include <unistd.h>
#include <sys/stat.h>
#include "glinerec.h"
#ifdef HAVE_GSL
#include "gsl.h"
#endif


namespace glinerec {

    ////////////////////////////////////////////////////////////////
    // bitmap classifier
    ////////////////////////////////////////////////////////////////

    typedef long long uint64;

    int bitcount0(uint64 v) {
        int total = 0;
        for(int i=0;i<64;i++) {
            if(v&1) total++;
            v>>=1;
        }
        return total;
    }

    static int counts16[65536];
    int counts16_init() {
        for(int i=0;i<65536;i++)
            counts16[i] = bitcount0(i);
        return 0;
    }

    int init_0 = counts16_init();

    int bitcount(uint64 v) {
        // this is about twice as fast as bitcount1
        return counts16[v&0xffff]+counts16[(v>>16)&0xffff]+
            counts16[(v>>32)&0xffff]+counts16[(v>>48)&0xffff];
    }

    struct bitvec {
        enum { bpw=64 };
        int nwords;
        uint64 *data;
        bitvec() {
            nwords = 0;
            data = 0;
        }
        ~bitvec() {
            if(data) delete [] data;
            data = 0;
            nwords = 0;
        }
        bitvec(const bitvec &other) {
            nwords = other.nwords;
            data = new uint64[nwords];
            memcpy(data,other.data,nwords * sizeof *data);
        }
        void operator=(const bitvec &other) {
            if(data) delete [] data;
            nwords = other.nwords;
            data = new uint64[nwords];
            memcpy(data,other.data,nwords * sizeof *data);
        }
        void reserve_bits(int nbits) {
            if(data) delete [] data;
            nwords = (nbits+bpw-1)/bpw;
            data = new uint64[nwords];
        }
        void reserve_words(int n) {
            if(data) delete [] data;
            nwords = n;
            data = new uint64[nwords];
        }
        template <class T>
        void set(T *v,int nbits,T threshold) {
            reserve_bits(nbits);
            uint64 word = 0xf0f0f0f0f0f0f0f0LLU;
            int index = 0;
            int bitindex = 0;
            for(int i=0;i<nbits;i++) {
                word <<= 1;
                if(v[i]>threshold) word |= 1;
                bitindex++;
                if(bitindex==bpw) {
                    data[index++] = word;
                    word = 0;
                    bitindex = 0;
                }
            }
            if(bitindex>0) {
                word <<= (bpw-bitindex);
                data[index++] = word;
            }
        }
        template <class T>
        void set(narray<T> &v) {
            set(&v(0),v.length(),(min(v)+max(v))/2);
        }
        template <class T>
        void get(T *v,int nbits) {
            uint64 word = 0;
            int index = 0;
            int bitindex = 0;
            for(int i=0;i<nbits;i++) {
                if(bitindex==0) {
                    word = data[index++];
                    bitindex = bpw;
                }
                v[i] = !!(word&0x8000000000000000LLU);
                word <<= 1;
                bitindex--;
            }
        }
        int countbits() {
            int total = 0;
            for(int i=0;i<nwords;i++)
                total += bitcount(data[i]);
            return total;
        }
        int dist(bitvec &other) {
            int total = 0;
            int n = min(nwords,other.nwords);
            uint64 *p = data, *q = other.data;
            for(int i=0;i<n;i++)
                total += bitcount((*p++) ^ (*q++));
            return total;
        }
    };

    void bitvec_write(FILE *stream,bitvec &v) {
        magic_write(stream,"BV");
        CHECK(unsigned(v.nwords)<1000000);
        scalar_write(stream,v.nwords);
        CHECK(fwrite(v.data,sizeof *v.data,v.nwords,stream)==
              unsigned(v.nwords));
    }

    void bitvec_read(FILE *stream,bitvec &v) {
        magic_read(stream,"BV");
        int nwords;
        scalar_read(stream,nwords);
        CHECK(unsigned(nwords)<1000000);
        v.reserve_words(nwords);
        CHECK(fread(v.data,sizeof *v.data,v.nwords,stream)==
              unsigned(v.nwords));
    }

    void display_bits(bitvec &v,int w,int h,const char *where) {
        bytearray image(w,h);
        image.fill(0);
        v.get(&image.at1d(0),w*h);
        for(int i=0;i<w*h;i++)
            if(image.at1d(i)) image.at1d(i) = 255;
        dshow(image,where);
    }

    struct BitDataset : IExtDataset {
        int nfeat;
        objlist<bitvec> prototypes;
        intarray classes;
        BitDataset() {
            nfeat = -1;
        }
        virtual int nsamples() {
            return classes.length();
        }
        virtual int nclasses() {
            return max(classes)+1;
        }
        virtual int nfeatures() {
            return nfeat;
        }
        virtual void input(floatarray &v,int i) {
            v.resize(nfeat);
            prototypes(i).get(&v[0],nfeat);
        }
        virtual int cls(int i) {
            return classes(i);
        }
        virtual int id(int i) {
            return i;
        }
        virtual void add(floatarray &v,int c) {
            if(nfeat<0) nfeat = v.length();
            CHECK(v.length()==nfeat);
            prototypes.push().set(v);
            classes.push(c);
        }
        virtual void add(floatarray &ds,intarray &cs) {
            floatarray v;
            for(int i=0;i<ds.dim(0);i++) {
                rowget(v,ds,i);
                add(v,cs(i));
            }
        }
        virtual void clear() {
            prototypes.clear();
            classes.clear();
        }
        virtual void save(FILE *stream) {
            magic_write(stream,"bitdataset");
            scalar_write(stream,nfeat);
            narray_write(stream,classes);
            for(int i=0;i<prototypes.length();i++)
                bitvec_write(stream,prototypes(i));
        }
        virtual void load(FILE *stream) {
            magic_read(stream,"bitdataset");
            scalar_read(stream,nfeat);
            narray_read(stream,classes);
            prototypes.clear();
            for(int i=0;i<prototypes.length();i++)
                bitvec_read(stream,prototypes.push());
        }
    };

    struct bitmap : bitvec {
        short w,h;
        template <class T>
        void set_bitmap(narray<T> &other) {
            CHECK(other.rank()==2);
            w = other.dim(0);
            h = other.dim(1);
            set(&other[0],w*h);
        }
        template <class T>
        void get_bitmap(narray<T> &other) {
            other.resize(w,h);
            get(&other[0],w*h);
        }
    };

    void bitmap_write(FILE *stream,bitmap &v) {
        magic_write(stream,"BM");
        CHECK(unsigned(v.nwords)<1000000);
        scalar_write(stream,v.nwords);
        scalar_write(stream,v.w);
        scalar_write(stream,v.h);
        CHECK(fwrite(v.data,sizeof *v.data,v.nwords,stream)==
              unsigned(v.nwords));
    }

    void bitmap_read(FILE *stream,bitmap &v) {
        magic_read(stream,"BM");
        int nwords;
        scalar_read(stream,nwords);
        CHECK(unsigned(nwords)<1000000);
        v.reserve_words(nwords);
        scalar_read(stream,v.w);
        scalar_read(stream,v.h);
        CHECK(fread(v.data,sizeof *v.data,v.nwords,stream)==
              unsigned(v.nwords));
    }

    struct BitNN : IBatchDense {
        int nfeat;
        objlist<bitvec> prototypes;
        intarray classes;
        BitNN() {
            pdef("k",1,"number of nearest neighbors");
            nfeat = 0;
        }
        const char *name() {
            return "bit";
        }
        void info(int depth,FILE *stream) {
            iprintf(stream,depth,"BitNN\n");
            pprint(stream,depth);
            int k = pgetf("k");
            iprintf(stream,depth,"nfeat %d nprotos %d nclasses %d k %d\n",
                    nfeat,prototypes.length(),max(classes)+1,k);
        }
        void save(FILE *stream) {
            psave(stream);
            narray_write(stream,classes);
            for(int i=0;i<classes.length();i++) {
                bitvec_write(stream,prototypes[i]);
            }
        }
        void load(FILE *stream) {
            pload(stream);
            narray_read(stream,classes);
            prototypes.resize(classes.length());
            for(int i=0;i<classes.length();i++) {
                bitvec_read(stream,prototypes[i]);
            }
        }
        int nfeatures() {
            return nfeat;
        }
        int nclasses() {
            return max(classes)+1;
        }
        void print() {
            printf("<BitNN #protos %d>\n",prototypes.length());
        }
        void train_dense(IDataset &ds) {
            floatarray v;
            for(int i=0;i<ds.nsamples();i++) {
                ds.input(v,i);
                train1(v,ds.cls(i));
            }
        }
        void train1(floatarray &v,int c) {
            if(nfeat==0) nfeat = v.length();
            else CHECK(nfeat==v.length());
            prototypes.push().set(v);
            classes.push(c);
        }
        float outputs_dense(floatarray &result,floatarray &v) {
            floatarray p;
            int k = pgetf("k");
            bitvec bv;
            bv.set(v);
            int n = prototypes.length();
            floatarray dists(n);
#pragma omp parallel for
            for(int j=0;j<n;j++)
                dists(j) = prototypes[j].dist(bv);
            knn_posterior(p,classes,dists,k);
            int nearest = argmin(dists);
            float cost = dists(nearest)/10.0;
            result = p;
            return cost;
        }
    };

    void init_glbits() {
        component_register<BitNN>("bitnn");
        component_register<BitDataset>("bitdataset");
    }
}
