// classifier implementations for glinerec

#include <unistd.h>
#include <sys/stat.h>
#include "glinerec.h"
#ifdef HAVE_GSL
#include "gsl.h"
#endif

namespace glinerec {
    // extern param_int csize_param;
    using namespace colib;
    using namespace iulib;
    using namespace narray_ops;
    using namespace narray_io;

    ////////////////////////////////////////////////////////////////
    // utility functions for classifiers
    ////////////////////////////////////////////////////////////////

    void checknan(floatarray &v) {
        int n = v.length1d();
        for(int i=0;i<n;i++)
            CHECK(!isnan(v.unsafe_at1d(i)));
    }

    template <class T>
    int binsearch(narray<T> &v,T x) {
        int n = v.length();
        if(n<1) return -1;
        int lo = 0;
        int hi = v.length();
        if(n>2) {               // quick sanity check
            int i = lrand48()%(n-1), j = i+(lrand48()%(n-i));
            CHECK(v(i)<=v(j));
        }
        for(;;) {
            int mean = (lo+hi)/2;
            if(mean==lo) return mean;
            float value = v(mean);
            if(value==x) return mean;
            else if(value<x) lo = mean;
            else hi = mean;
        }
    }

    template <class T>
    bool bincontains(narray<T> &v,T x) {
        int index = binsearch(v,x);
        return v(index)==x;
    }

    int count_samples(intarray &classes) {
        int count = 0;
        for(int i=0;i<classes.length();i++)
            if(classes(i)>=0) count++;
        return count;
    }

    void weighted_sample(intarray &samples,floatarray &weights,int n) {
        floatarray cs;
        cs = weights;
        for(int i=1;i<cs.length();i++)
            cs(i) += cs(i-1);
        cs /= max(cs);
        samples.clear();
        for(int i=0;i<n;i++) {
            float value = drand48();
            int where = binsearch(cs,value);
            samples.push(where);
        }
    }

    double perplexity(floatarray &weights) {
        floatarray w;
        w = weights;
        w /= sum(w);
        double total = 0.0;
        for(int i=0;i<w.length();i++) {
            float value = w(i);
            total += value * log(value);
        }
        return exp(-total);
    }

    double entropy(floatarray &a) {
        double z = sum(a);
        double total = 0.0;
        for(int i=0;i<a.length();i++) {
            double p = a[i]/z;
            if(p<1e-8) continue;
            total += p * log(p);
        }
        return -total;
    }

    double nearest_neighbor_error(IDataset &data,int ntrials=1000) {
        int total = 0;
        ntrials = min(data.nsamples(),ntrials);
        for(int i=0;i<data.nsamples();i++) {
            // FIXME use sampling without replacement
            int i = lrand48()%data.nsamples();
            floatarray u;
            data.input1d(u,i);
            int n = data.nsamples();
            floatarray dists(n);
#pragma omp parallel for shared(dists)
            for(int j=0;j<data.nsamples();j++) {
                if(j==i) {
                    dists(j)= 1e30;
                } else {
                    floatarray v;
                    data.input1d(v,j);
                    dists(j) = dist2squared(u,v);
                }
            }
            int index = argmin(dists);
            if(data.cls(index)!=data.cls(i)) total++;
        }
        return total/double(data.nsamples());
    }

    double nearest_neighbor_error(IDataset &training,IDataset &testing) {
        int total = 0;
        for(int i=0;i<testing.nsamples();i++) {
            floatarray u;
            testing.input1d(u,i);

            int n = training.nsamples();
            floatarray dists(n);
#pragma omp parallel for shared(dists)
            for(int j=0;j<training.nsamples();j++) {
                floatarray v;
                training.input1d(v,j);
                dists(j) = dist2squared(u,v);
            }
            int index = argmin(dists);
            if(training.cls(index)!=testing.cls(i)) total++;
        }
        return total/double(testing.nsamples());
    }

    float estimate_errors(IModel &classifier,IDataset &ds,int n=1000000) {
        floatarray v;
        floatarray out;
        int errors = 0;
        int count = 0;
        for(int i=0;i<ds.nsamples();i++) {
            int cls = ds.cls(i);
            if(cls==-1) continue;
            ds.input1d(v,i);
            int pred = classifier.classify(v);
            count++;
            if(pred!=cls) errors++;
        }
        return errors/float(count);
    }

    float estimate_errors_sampled(IModel &classifier,floatarray &data,intarray &classes,int n=1000) {
        int errors = 0;
        for(int i=0;i<n;i++) {
            floatarray v;
            int row;
            do {row = lrand48()%classes.length();} while(classes(row)==-1);
            rowget(v,data,row);
            int pred = classifier.classify(v);
            if(pred!=classes(row)) errors++;
        }
        return errors/float(n);
    }

    void confusion_matrix(intarray &confusion,IModel &classifier,IDataset &ds) {
        int npred = classifier.nclasses();
        int nclasses = ds.nclasses();
        confusion.resize(npred,nclasses);
        confusion.fill(0);
#pragma omp parallel for
        for(int i=0;i<ds.nsamples();i++) {
            floatarray v;
            int cls = ds.cls(i);
            if(cls==-1) continue;
            ds.input1d(v,i);
            int pred = classifier.classify(v);
            confusion(pred,cls)++;
        }
    }
    /// Compute a list of confusions in an n x 3 array.
    /// list(k,0) is the # errors
    /// list(k,1) is the true class
    /// list(k,2) is the predicted
    /// Orders is from largest # confusions to smallest.
    /// The total number of errors is returned.

    template <class T>
    void rowtruncate(narray<T> &a,int n) {
        CHECK(n<=a.dim(0));
        narray<T> temp(n,a.dim(1));
        narray<T> v;
        for(int i=0;i<temp.dim(0);i++) {
            rowget(v,a,i);
            rowput(temp,i,v);
        }
        a.move(temp);
    }
    int compute_confusions(intarray &list,intarray &confusion) {
        list.resize(confusion.dim(0)*confusion.dim(1),3);
        list = 0;
        int row = 0;
        float total = 0;
        for(int i=0;i<confusion.dim(0);i++) {
            for(int j=0;j<confusion.dim(1);j++) {
                if(confusion(i,j)==0) continue;
                if(i==j) continue;
                total += confusion(i,j);
                list(row,0) = -confusion(i,j);
                list(row,1) = i;
                list(row,2) = j;
                row++;
            }
        }
        intarray perm;
        rowsort(perm,list);
        rowpermute(list,perm);
        {
            int i=0;
            while(i<list.dim(0) && list(i,0)!=0) i++;
            rowtruncate(list,i);
        }
        for(int i=0;i<list.dim(0);i++)
            list(i,0) = -list(i,0);
        return total;
    }
    void confusion_matrix(intarray &confusion,IModel &classifier,floatarray &data,intarray &classes) {
        int npred = classifier.nclasses();
        int nclasses = max(classes) + 1;
        confusion.resize(npred,nclasses);
        confusion.fill(0);
#pragma omp parallel for
        for(int i=0;i<data.dim(0);i++) {
            floatarray v;
            if(classes(i)==-1) continue;
            rowget(v,data,i);
            int pred = classifier.classify(v);
            confusion(pred,classes(i))++;
        }
    }
    float confusion_error(intarray &confusion) {
        double total = 0.0;
        double err = 0.0;
        for(int i=0;i<confusion.dim(0);i++) {
            for(int j=0;j<confusion.dim(1);j++) {
                total += confusion(i,j);
                if(i!=j) err += confusion(i,j);
            }
        }
        return err/total;
    }

    void sparsify(floatarray &a,int n) {
        NBest nbest(n);
        for(int i=0;i<a.length1d();i++)
            nbest.add(i,fabs(a.at1d(i)));
        float threshold = nbest.value(n-1);
        for(int i=0;i<a.length1d();i++)
            if(fabs(a.at1d(i))<threshold) a.at1d(i) = 0;
    }

    void confusion_print(intarray &confusion) {
        intarray skip(confusion.dim(0));
        for(int i=0;i<confusion.dim(0);i++)
            skip(i) = (rowsum(confusion,i)==0);
        printf("           ");
        for(int j=0;j<confusion.dim(1);j++ ) {
            if(skip(j)) continue;
            printf(" %4d",j);
        }
        printf("\n\n");
        for(int i=0;i<confusion.dim(0);i++) {
            if(skip(i)) continue;
            printf("conf %3d : ",i);
            for(int j=0;j<confusion.dim(1);j++ ) {
                if(skip(j)) continue;
                printf(" %4d",confusion(i,j));
            }
            printf("\n");
        }
    }
    void print_confusion(IModel &classifier,floatarray &vs,intarray &cs) {
        intarray cm;
        confusion_matrix(cm,classifier,vs,cs);
        confusion_print(cm);
        printf("error %g\n",confusion_error(cm));
    }

    void n_smallest(intarray &index,floatarray &v,int n) {
        quicksort(index,v);
        index.truncate(n);
    }

    void n_largest(intarray &index,floatarray &v,int n) {
        floatarray temp;
        temp.copy(v);
        neg(temp);
        quicksort(index,temp);
        index.truncate(n);
    }

    ////////////////////////////////////////////////////////////////
    // k-nearest neighbor classifier
    ////////////////////////////////////////////////////////////////

    // param_int show_knn("show_knn",0,"show knn matches for debugging");

    struct KnnClassifier : virtual IBatch {
        int ncls;
        floatarray vectors;
        intarray classes;
        int ndim;
        float min_dist;

        KnnClassifier() {
            ncls = 0;
            ndim = -1;
            ncls = 0;
            min_dist = -1;
            pdef("k",1,"number of nearest neighbors");
            persist(vectors,"vectors");
            persist(classes,"classes");
        }
        const char *name() {
            return "knn";
        }
        void info(int depth,FILE *stream) {
            iprintf(stream,depth,"k-NN Classifier\n");
            pprint(stream,depth);
            int k = pgetf("k");
            iprintf(stream,depth,"k=%d ndim=%d nclasses=%d\n",k,ndim,nclasses());
        }
        void clear() {
            ncls = 0;
            vectors.clear();
            classes.clear();
        }
        void dealloc() {
            ncls = 0;
            vectors.dealloc();
            classes.dealloc();
        }
        int nfeatures() {
            return vectors.dim(1);
        }
        int nclasses() {
            return max(classes)+1;
        }
        float complexity() {
            return vectors.dim(0);
        }
        int nprotos() {
            return vectors.dim(0);
        }
        void getproto(floatarray &v,int i,int variant) {
            rowget(v,vectors,i);
        }
        void train(IDataset &ds) {
            floatarray v;
            for(int i=0;i<ds.nsamples();i++) {
                ds.input1d(v,i);
                train1(v,ds.cls(i));
            }
        }
        void train1(floatarray &v,int c) {
            CHECK(min(v)>-100 && max(v)<100);
            ASSERT(valid(v));
            ASSERT(v.rank()==1);
            CHECK(c>=0);
            if(ndim<0) ndim = v.dim(0);
            else CHECK(v.dim(0)==ndim);
            rowpush(vectors,v);
            classes.push(c);
            if(c>=ncls) ncls = c+1;
            ASSERT(vectors.dim(0)==classes.length());
        }
        float outputs(OutputVector &result,floatarray &v) {
            int k = pgetf("k");
            CHECK(min(v)>-100 && max(v)<100);
            CHECK(v.dim(0)==ndim);
            NBest nbest(k);
            for(int i=0;i<vectors.dim(0);i++) {
                double d = rowdist_euclidean(vectors,i,v);
                if(fabs(d)<min_dist) continue;
                nbest.add(i,-d);
            }
            result.clear();
            for(int i=0;i<nbest.length();i++) {
                result(classes[nbest[i]])++;
            }
            result.normalize();
            return 0.0;
        }
    };

#if 0
    ////////////////////////////////////////////////////////////////
    // bucket dispatching classifier
    ////////////////////////////////////////////////////////////////

    inline void pad_to(floatarray &a,int w1,int h1) {
        int w = a.dim(0), h = a.dim(1);
        CHECK(w<=w1 && h<=h1);
        floatarray temp(w1,h1);
        temp = 0;
        int dx = w1/2;
        int dy = h1/2;
        for(int i=0;i<w;i++) {
            for(int j=0;j<h;j++) {
                temp(i+dx,j+dy) = a(i,j);
            }
        }
        a = temp;
    }

    struct BinningClassifier : virtual IBatchDense {
        intarray widths,heights;
        narray<autodel<IModel> > models;
        BinningClassifier() {
            pdef("q",5,"bin quantization");
            pdef("model","mlp","underlying model");
        }
        virtual const char *name() {
            return "BinningClassifier";
        }
        virtual int nmodels() {
            return models.length();
        }
        virtual void setModel(IModel *model,int i) {
            models[i] = model;
        }
        virtual IComponent &getModel(int i) {
            return *models[i];
        }
        void bin(floatarray &temp,floatarray &v) {
            int q = pgetf("q");
            CHECK(v.rank()==2);
            int w = v.dim(0);
            int h = v.dim(1);
            int w1 = q*((w+q-1)/q);
            int h1 = q*((h+q-1)/q);
            temp = v;
            pad_to(temp,w1,h1);
        }
        float outputs(OutputVector &result,floatarray &v) {
            floatarray temp;
            bin(temp,v);
            int w1 = temp.dim(0), h1 = temp.dim(1);
            for(int i=0;i<models.length();i++) {
                if(widths[i]==w1 && heights[i]==h1) {
                    return models[i]->outputs(result,temp);
                }
            }
            result.clear();
            return 1e30;
        }
        void add(floatarray &v,int c) {
            floatarray temp;
            bin(temp,v);
            int w1 = temp.dim(0), h1 = temp.dim(1);
            for(int i=0;i<models.length();i++) {
                if(widths[i]==w1 && heights[i]==h1) {
                    models[i]->add(temp,c);
                    return;
                }
            }
            widths.push(w1);
            heights.push(h1);
            models.push() = make_model(pget("model"));
            models.last()->add(v,c);
        }
        virtual void updateModel() {
            for(int i=0;i<models.length();i++)
                models[i]->updateModel();
        }
        virtual void train(IDataset &dataset) {
            throw Unimplemented();
        }
    };
#endif

    ////////////////////////////////////////////////////////////////
    // clustering classifier
    ////////////////////////////////////////////////////////////////

    struct EnetClassifier : virtual IBatchDense {
        narray<floatarray> vectors;
        narray<intarray> classes;
        narray<intarray> counts;
        int ndim,ncls;
        int dtype;
        float min_dist;
        float offset;

        EnetClassifier() {
            ncls = 0;
            ndim = -1;
            ncls = 0;
            min_dist = -1;
            pdef("eps",5.0,"max dist for new cluster");
            pdef("avg",1,"average new vectors with old ones in cluster");
            pdef("k",1,"number of nearest neighbors");
            pdef("verbose",0,"verbose output");
            pdef("dtype",1,"distance type");
            pdef("offset",0.01,"probabilistic offset");
            pdef("fuzz",0.5,"initial smoothing");
            persist(vectors,"vectors");
            persist(classes,"classes");
        }
        const char *name() {
            return "knn";
        }
        int nprotos() {
            return vectors.dim(0);
        }
        void getproto(floatarray &v,int i,int variant) {
            v = vectors(i);
        }
        void info(int depth,FILE *stream) {
            iprintf(stream,depth,"Clustering Classifier\n");
            pprint(stream,depth);
            int k = pgetf("k");
            iprintf(stream,depth,"k=%d nclusters=%d ndim=%d nclasses=%d\n",k,vectors.dim(0),
                ndim,nclasses());
        }
        void clear() {
            ncls = 0;
            vectors.clear();
            classes.clear();
            counts.clear();
        }
        void dealloc() {
            ncls = 0;
            vectors.dealloc();
            classes.dealloc();
            counts.dealloc();
        }
        int nfeatures() {
            return vectors(0).length();
        }
        int nclasses() {
            return ncls;
        }
        float complexity() {
            return vectors.dim(0);
        }
        float distance(floatarray &v,floatarray &proto) {
            double result = 0.0;
            switch(dtype) {
            case 0:
                for(int i=0;i<v.length();i++) {
                    float delta = v[i]-proto[i];
                    result += (delta*delta);
                }
                return result;
            case 1:
                for(int i=0;i<v.length();i++) {
                    float delta = v[i]-proto[i];
                    result += -log(1.0-fabs(delta)+offset);
                    if(isnan(result)) {
                        printf("ERROR %g %g / %g\n",v[i],proto[i],delta);
                        abort();
                    }
                }
                return result;
            }
            throw "bad dtype";
        }
        void train_dense(IDataset &ds) {
            floatarray v;
            for(int i=0;i<ds.nsamples();i++) {
                ds.input1d(v,i);
                train1(v,ds.cls(i));
            }
        }
        void check(floatarray &v,int c=99999999) {
            if(dtype==1) {
                for(int i=0;i<v.length();i++)
                    CHECK(v[i]==0 || v[i]==1);
            } else {
                for(int i=0;i<v.length();i++)
                    CHECK(v[i]>=-0.001 || v[i]<=1.001);
            }
            ASSERT(valid(v));
            if(ndim<0) ndim = v.length();
            else CHECK(v.length()==ndim);
            if(c!=99999999) {
                CHECK(c>=0);
            }
        }
        void add_count(intarray &classes,intarray &counts,int c) {
            for(int i=0;i<classes.length();i++) {
                if(classes(i)==c) {
                    counts(i)++;
                    return;
                }
            }
            classes.push(c);
            counts.push(1);
        }
        void train1(floatarray &v,int c) {
            dtype = pgetf("dtype");
            offset = pgetf("offset");
            check(v,c);
            floatarray distances(vectors.dim(0));
            float eps = pgetf("eps");
            int best = -1;
            if(vectors.dim(0)>0) {
#pragma omp parallel for
                for(int i=0;i<vectors.dim(0);i++)
                    distances(i) = distance(v,vectors(i));
                best = argmin(distances);
                if(pgetf("verbose")) printf("best cost %g\n",distances(best));
            }
            if(best>=0 && distances(best)<eps) {
                if(pgetf("avg")) {
                    using namespace narray_ops;
                    int n = sum(counts(best));
                    vectors(best) *= n;
                    vectors(best) += v;
                    vectors(best) *= 1.0/(n+1);
                }
                add_count(classes(best),counts(best),c);
            } else {
                floatarray temp;
                temp = v;
                float fuzz = pgetf("fuzz");
                gauss2d(temp,fuzz,fuzz);
                temp /= max(temp);
                vectors.push() = temp;
                add_count(classes.push(),counts.push(),c);
            }
            if(c>=ncls) ncls = c+1;
            ASSERT(vectors.dim(0)==classes.length());
        }
        float outputs_dense(floatarray &result,floatarray &v) {
            dtype = pgetf("dtype");
            offset = pgetf("offset");
            check(v);
            int k = pgetf("k");
            floatarray distances(vectors.dim(0));
#pragma omp parallel for
            for(int i=0;i<vectors.dim(0);i++)
                distances(i) = distance(v,vectors(i));

            NBest nbest(k);
            for(int i=0;i<vectors.dim(0);i++)
                nbest.add(i,-distances(i));

            result.resize(ncls);
            fill(result,0);
            for(int i=0;i<nbest.length();i++) {
                int k = nbest[i];
                for(int j=0;j<classes[k].length();j++)
                    result(classes[k][j]) += counts[k][j];
            }
            result /= sum(result);

            if(dactive()) {
                int r = sqrt(ndim);
                floatarray temp;
                temp = vectors(nbest[0]);
                if(temp.rank()==1) temp.reshape(r,r);
                dshown(temp,"d");
                temp = v;
                if(temp.rank()==1) temp.reshape(r,r);
                dshown(temp,"c");
            }
            return nbest.value(0);
        }
    };

    ////////////////////////////////////////////////////////////////
    // MLP classifier
    ////////////////////////////////////////////////////////////////

    // sigmoid functions for neural networks

    inline float sigmoid(float x) {
        return 1.0 / (1.0+exp(-min(max(x,-20.0),20.0)));
    }

    inline float dsigmoidy(float y) {
        return y*(1-y);
    }

    // logarithmically spaced samples in an interval

    inline float logspace(int i,int n,float lo,float hi) {
        return exp((i/float(n-1)) * (log(hi)-log(lo)) + log(lo));
    }

    // random samples from the normal density

    void mvmul0(floatarray &out,floatarray &a,floatarray &v) {
        int n = a.dim(0);
        int m = a.dim(1);
        CHECK(m==v.length());
        out.resize(a.dim(0));
        out.fill(0);
        for(int j=0;j<m;j++) {
            float value = v.unsafe_at(j);
            if(value==0) continue;
            for(int i=0;i<n;i++)
                out.unsafe_at(i) += a.unsafe_at(i,j) * value;
        }
    }

    void vmmul0(floatarray &out,floatarray &v,floatarray &a) {
        int n = a.dim(0);
        int m = a.dim(1);
        CHECK(n==v.length());
        out.resize(a.dim(1));
        out.fill(0);
        for(int i=0;i<n;i++) {
            float value = v(i);
            if(value==0) continue;
            for(int j=0;j<m;j++)
                out.unsafe_at(j) += a.unsafe_at(i,j) * value;
        }
    }

    void matmul(floatarray &out,floatarray &a,floatarray &b) {
        if(a.rank()==2) {
            if(b.rank()==2) {
                throw "bad ranks in matmul";
            } else if(b.rank()==1) {
                CHECK(a.dim(1)==b.dim(0));
                out.resize(a.dim(0));
                for(int i=0;i<a.dim(0);i++) {
                    double total = 0.0;
#ifdef SAFE
                    for(int j=0;j<a.dim(1);j++)
                        total += a(i,j) * b(j);
#else
                    int n = a.dim(1);
                    float *ap = &a(i,0);
                    float *bp = &b(0);
                    for(int j=0;j<n;j++) total += (*ap++) * (*bp++);
#endif
                    out(i) = total;
                }
            } else {
                throw "bad ranks in matmul";
            }
        } else if(a.rank()==1) {
            if(b.rank()==2) {
                CHECK(b.dim(0)==a.dim(0));
                out.resize(b.dim(1));
                for(int j=0;j<b.dim(1);j++) {
                    double total = 0.0;
#ifdef SAFE
                    for(int i=0;i<b.dim(0);i++)
                        total += a(i) * b(i,j);
#else
                    float *ap = &a(0);
                    float *bp = &b(0,j);
                    int delta = &b(1,j)-&b(0,j);
                    int n = b.dim(0);
                    for(int i=0;i<n;i++) {
                        total += *ap * *bp;
                        ap++;
                        bp += delta;
                    }
#endif
                    out(j) = total;
                }
            } else if(b.rank()==1) {
                throw "bad ranks in matmul";
            } else {
                throw "bad ranks in matmul";
            }
        } else {
            throw "bad ranks in matmul";
        }
    }

    int count_zeros(floatarray &a) {
        int n = a.length1d();
        int count = 0;
        for(int i=0;i<n;i++)
            if(a.unsafe_at1d(i)==0) count++;
        return count;
    }

    void outer_add(floatarray &a,floatarray &u,floatarray &v,float eps) {
        int n = a.dim(0);
        int m = a.dim(1);
        CHECK(n==u.length());
        CHECK(m==v.length());
        if(count_zeros(u)>=count_zeros(v)) {
            for(int i=0;i<n;i++) {
                if(u.unsafe_at(i)==0) continue;
                for(int j=0;j<m;j++) {
                    a.unsafe_at(i,j) += eps * u.unsafe_at(i) * v.unsafe_at(j);
                }
            }
        } else {
            for(int j=0;j<m;j++) {
                if(v.unsafe_at(j)==0) continue;
                for(int i=0;i<n;i++) {
                    a.unsafe_at(i,j) += eps * u.unsafe_at(i) * v.unsafe_at(j);
                }
            }
        }
    }

    struct MlpClassifier : virtual IBatchDense {
    public:
        floatarray w1,b1,w2,b2;
        float eta;
        float cv_error;
        float nn_error;
        bool crossvalidate;

        MlpClassifier() {
            pdef("eta",0.5,"default learning rate");
            pdef("eta_init",0.5,"initial eta");
            pdef("eta_varlog",1.5,"eta variance in lognormal");
            pdef("hidden_varlog",1.2,"nhidden variance in lognormal");
            pdef("rounds",8,"number of training rounds");
            pdef("miters",8,"number of presentations in multiple of training set");
            pdef("nensemble",4,"number of mlps in ensemble");
            pdef("hidden_min",5,"minimum number of hidden units");
            pdef("hidden_lo",20,"minimum number of hidden units at start");
            pdef("hidden_hi",80,"maximum number of hidden units at start");
            pdef("hidden_max",300,"maximum number of hidden units");
            pdef("sparse",-1,"sparsify the hidden layer");
            pdef("cv_split",0.8,"cross validation split");
            pdef("cv_max",5000,"max # samples to use for cross validation");
            pdef("normalization",-1,"kind of normalization of the input");
            pdef("noopt",0,"disable optimization search");
            pdef("crossvalidate",1,"perform crossvalidation");
            eta = pgetf("eta");
            cv_error = 1e30;
            nn_error = 1e30;
            persist(w1,"w1");
            persist(b1,"b1");
            persist(w2,"w2");
            persist(b2,"b2");
        }

        void normalize(floatarray &v) {
            float kind = pgetf("normalization");
            if(kind<0) return;
            if(kind==1) {
                double total = 0.0;
                for(int i=0;i<v.length();i++) total += fabs(v[i]);
                for(int i=0;i<v.length();i++) v[i] /= total;
            } else if(kind==2) {
                double total = 0.0;
                for(int i=0;i<v.length();i++) total += sqr(v[i]);
                total = sqrt(total);
                for(int i=0;i<v.length();i++) v[i] /= total;
            } else if(kind<999) {
                double total = 0.0;
                for(int i=0;i<v.length();i++) total += pow(v[i],kind);
                total = pow(total,1.0/kind);
                for(int i=0;i<v.length();i++) v[i] /= total;
            } else if(kind==999) {
                double total = 0.0;
                for(int i=0;i<v.length();i++) if(fabs(v[i])>total) total = fabs(v[i]);
                for(int i=0;i<v.length();i++) v[i] /= total;
            } else {
                throw "bad normalization in mlp";
            }
        }

        void info(int depth,FILE *stream) {
            iprintf(stream,depth,"MLP\n");
            pprint(stream,depth);
            iprintf(stream,depth,"ninput %d nhidden %d noutput %d\n",w1.dim(1),w1.dim(0),w2.dim(0));
            if(w1.length()>0 && w2.length()>0) {
                iprintf(stream,depth,"w1 [%g,%g] b1 [%g,%g]\n",min(w1),max(w1),min(b1),max(b1));
                iprintf(stream,depth,"w2 [%g,%g] b2 [%g,%g]\n",min(w2),max(w2),min(b2),max(b2));
            }
        }

        int nfeatures() {
            return w1.dim(1);
        }
        int nhidden() {
            return w1.dim(0);
        }

        const char *name() {
            return "mlp";
        }
        float complexity() {
            return w1.dim(0);
        }

        int nclasses() {
            return w2.dim(0);
        }

        void copy(MlpClassifier &other) {
            w1.copy(other.w1);
            b1.copy(other.b1);
            w2.copy(other.w2);
            b2.copy(other.b2);
        }

        float outputs_dense(floatarray &result,floatarray &x_raw) {
            CHECK_ARG(x_raw.length()==w1.dim(1));
            floatarray z;
            int sparse = pgetf("sparse");
            floatarray y,x;
            x.copy(x_raw);
            mvmul0(y,w1,x);
            y += b1;
            for(int i=0;i<y.length();i++) y(i) = sigmoid(y(i));
            if(sparse>0) sparsify(y,sparse);
            mvmul0(z,w2,y);
            z += b2;
            for(int i=0;i<z.length();i++) z(i) = sigmoid(z(i));
            result = z;
            return fabs(sum(z)-1.0);
        }

        void changeHidden(int newn) {
            MlpClassifier temp;
            int ninput = w1.dim(1);
            int nhidden = w1.dim(0);
            int noutput = w2.dim(0);
            temp.initRandom(ninput,newn,noutput);
            for(int i=0;i<newn;i++) {
                if(i>=nhidden) {
                    for(int j=0;j<ninput;j++)
                        temp.w1(i,j) = rnormal(0.0,1.0);
                    temp.b1(i) = rnormal(0.0,1.0);
                } else {
                    for(int j=0;j<ninput;j++)
                        temp.w1(i,j) = w1(i,j);
                    temp.b1(i) = b1(i);
                }
            }
            for(int i=0;i<noutput;i++) {
                for(int j=0;j<newn;j++) {
                    if(j>=nhidden) {
                        temp.w2(i,j) = 1e-2*rnormal(0.0,1.0);
                    } else {
                        temp.w2(i,j) = w2(i,j);
                    }
                }
            }
            this->copy(temp);
        }

        void initData(IDataset &ds,int nhidden) {
            CHECK(nhidden>1 && nhidden<1000000);
            int ninput = ds.nfeatures();
            int noutput = ds.nclasses();
            w1.resize(nhidden,ninput);
            b1.resize(nhidden);
            w2.resize(noutput,nhidden);
            b2.resize(noutput);
            intarray indexes;
            rpermutation(indexes,ds.nsamples());
            floatarray v;
            for(int i=0;i<w1.dim(0);i++) {
                int row = indexes(i);
                ds.input1d(v,row);
                v /= sqr(norm(v));
                rowput(w1,i,v);
            }
            fill_random(b1,-1e-6,1e-6);
            fill_random(w2,-1.0/nhidden,1.0/nhidden);
            fill_random(b2,-1e-6,1e-6);
        }

        void initRandom(int ninput,int nhidden,int noutput) {
            w1.resize(nhidden,ninput);
            b1.resize(nhidden);
            w2.resize(noutput,nhidden);
            b2.resize(noutput);
            float range = 1.0/max(ninput,nhidden);
            fill_random(w1,-range,range);
            fill_random(b1,-range,range);
            fill_random(w2,-range,range);
            fill_random(b2,-range,range);
        }

        float crossValidatedError() {
            return cv_error;
        }

        // do a single stochastic gradient descent step

        void trainOne(floatarray &z,floatarray &target,floatarray &x,float eta) {
            CHECK_ARG(target.length()==w2.dim(0));
            CHECK_ARG(x.length()==w1.dim(1));

            int sparse = pgetf("sparse");
            int nhidden = this->nhidden();
            int noutput = nclasses();
            floatarray delta1(nhidden),delta2(noutput),y(nhidden);

            mvmul0(y,w1,x);
            y += b1;
            for(int i=0;i<nhidden;i++)
                y(i) = sigmoid(y(i));
            if(sparse>0) sparsify(y,sparse);
            mvmul0(z,w2,y);
            z += b2;
            for(int i=0;i<noutput;i++)
                z(i) = sigmoid(z(i));

            for(int i=0;i<noutput;i++)
                delta2(i) = (z(i)-target(i)) * dsigmoidy(z(i));
            vmmul0(delta1,delta2,w2);
            for(int i=0;i<nhidden;i++)
                delta1(i) = delta1(i) * dsigmoidy(y(i));

            outer_add(w2,delta2,y,-eta);
            for(int i=0;i<noutput;i++)
                b2(i) -= eta * delta2(i);
            outer_add(w1,delta1,x,-eta);
            for(int i=0;i<nhidden;i++)
                b1(i) -= eta * delta1(i);
        }

        void train_dense(IDataset &ds) {
            dsection("mlp");
            int nclasses = ds.nclasses();
            float miters = pgetf("miters");
            int niters = (ds.nsamples() * miters);
            niters = max(1000,min(10000000,niters));
            double err = 0.0;
            floatarray x,z,target(nclasses);
            int count = 0;
            for(int i=0;i<niters;i++) {
                int row = i%ds.nsamples();
#if 0
                int cls = ds.cls(row);
                if(cls<0) continue;
                target = 0;
                target(cls) = 1;
#else
                ds.output(target,row);
#endif
                ds.input1d(x,row);
                trainOne(z,target,x,eta);
                err += dist2squared(z,target);
                if(dactive()) {
                    floatarray v;
                    int r = sqrt(v.length());
                    v = x; v.reshape(r,r);
                    dshown(v,"a");
                    // v = y; v.reshape(v.length(),1);
                    // dshown(v,"cyY");
                    v = z; v.reshape(v.length(),1);
                    dshown(v,"cYy");
                    v = target; v.reshape(v.length(),1);
                    dshown(v,"cYY");
                }
                count++;
            }
            err /= count;
            debugf("training-detail","MlpClassifier n %d niters %d eta %g err %g\n",
                   ds.nsamples(),niters,eta,err);
        }

        void print() {
            printf("mlp ninput %d nhidden %d noutput %d\n",
                   nfeatures(),nhidden(),nclasses());
        }
    };

    ////////////////////////////////////////////////////////////////
    // MLP classifier with automatic rate adaptation, cross
    // validation and parallel training
    ////////////////////////////////////////////////////////////////

    struct AutoMlpClassifier : virtual MlpClassifier {

        AutoMlpClassifier() {
        }

        void train_dense(IDataset &ds) {
            pset("%nsamples",ds.nsamples());
            float split = pgetf("cv_split");
            int mlp_cv_max = pgetf("cv_max");
            if(crossvalidate) {
                // perform a split for cross-validation, making sure
                // that we don't have the same sample in both the
                // test and the training set (even if the data set
                // is the result of resampling)
                intarray test_ids,ids;
                for(int i=0;i<ds.nsamples();i++) ids.push(ds.id(i));
                uniq(ids);
                debugf("cvdetail","reduced %d ids to %d ids\n",ds.nsamples(),ids.length());
                shuffle(ids);
                int nids = int((1.0-split)*ids.length());
                nids = min(nids,mlp_cv_max);
                for(int i=0;i<nids;i++) test_ids.push(ids(i));
                quicksort(test_ids);
                intarray training,testing;
                for(int i=0;i<ds.nsamples();i++) {
                    int id = ds.id(i);
                    if(bincontains(test_ids,id))
                        testing.push(i);
                    else
                        training.push(i);
                }
                debugf("cvdetail","#training %d #testing %d\n",
                       training.length(),testing.length());
                pset("%ntraining",training.length());
                pset("%ntesting",testing.length());
                Datasubset trs(ds,training);
                Datasubset tss(ds,testing);
#if 0
                // too slow
                debugf("info","computing nn error\n");
                nn_error = nearest_neighbor_error(trs,tss);
                debugf("info","nn error %g (%d,%d)\n",nn_error,trs.nsamples(),tss.nsamples());
#endif
                trainBatch(trs,tss);
            } else {
#if 0
                int nids = min(int(mlp_cv_split*ds.nsamples()),mlp_cv_max);
                // too slow
                debugf("info","computing nn error\n");
                nn_error = nearest_neighbor_error(ds,nids);
                debugf("info","nn error %g (%d,%d)\n",nn_error,ds.nsamples()-nids,nids);
#endif
                trainBatch(ds,ds);
            }
        }

        void trainBatch(IDataset &ds,IDataset &ts) {
            float eta_init = pgetf("eta_init"); // 0.5
            float eta_varlog = pgetf("eta_varlog"); // 1.5
            float hidden_varlog = pgetf("hidden_varlog"); // 1.2
            int hidden_lo = pgetf("hidden_lo");
            int hidden_hi = pgetf("hidden_hi");
            int rounds = pgetf("rounds");
            int mlp_noopt = pgetf("noopt");
            int hidden_min = pgetf("hidden_min");
            int hidden_max = pgetf("hidden_max");
            CHECK(hidden_min>1 && hidden_max<1000000);
            CHECK(hidden_hi>=hidden_lo);
            CHECK(hidden_max>=hidden_min);
            CHECK(hidden_lo>=hidden_min && hidden_hi<=hidden_max);
            int nn = pgetf("nensemble");
            objlist<MlpClassifier> nets;
            nets.resize(nn);
            floatarray errs(nn);
            floatarray etas(nn);
            intarray index;
            float best = 1e30;
            int nclasses = ds.nclasses();

            floatarray v;
            for(int i=0;i<ds.nsamples();i++) {
                ds.input1d(v,i);
                CHECK(min(v)>-100 && max(v)<100);
            }
            CHECK(ds.nsamples()>=10 && ds.nsamples()<100000000);

            for(int i=0;i<nn;i++) {
                // nets(i).init(data.dim(1),logspace(i,nn,hidden_lo,hidden_hi),nclasses);
                if(w1.length()>0) {
                    nets(i).copy(*this);
                } else {
                    nets(i).initData(ds,logspace(i,nn,hidden_lo,hidden_hi));
                }
                etas(i) = rlognormal(eta_init,eta_varlog);
            }

            debugf("info","mlp training n %d nc %d\n",ds.nsamples(),nclasses);
            for(int round=0;round<rounds;round++) {
                errs.fill(-1);
#pragma omp parallel for
                for(int i=0;i<nn;i++) {
                    nets(i).pset("eta",etas(i));
                    nets(i).train(ds);
                    errs(i) = estimate_errors(nets(i),ts);
                    debugf("detail","net %d (%d/%d) %g %g %g\n",i,THREAD,NTHREADS,
                           errs(i),nets(i).complexity(),etas(i));
                }
                quicksort(index,errs);
                if(errs(index(0))<best) {
                    best = errs(index(0));
                    cv_error = best;
                    this->copy(nets(index(0)));
                    debugf("training-detail","best mlp update error %g %s\n",best,crossvalidate?"cv":"");
                    fflush(stdout);
                }
                if(!mlp_noopt) {
                    for(int i=0;i<nn/2;i++) {
                        int j = i+nn/2;
                        nets(index(j)).copy(nets(index(i)));
                        int n = nets(index(j)).nhidden();
                        int nn = min(max(hidden_min,int(rlognormal(n,hidden_varlog))),hidden_max);
                        nets(index(j)).changeHidden(nn);
                        etas(index(j)) = rlognormal(etas(index(i)),eta_varlog);
                    }
                }
                debugf("info","mlp round %d err %g nhidden %d\n",round,best,nhidden());
                pset("%error",best);
            }
        }

    };

    ////////////////////////////////////////////////////////////////
    // AdaBoost
    ////////////////////////////////////////////////////////////////

    void least_square(floatarray &xf,floatarray &Af,floatarray &bf) {
#ifdef GSL
        int N = Af.dim(1);
        gsl::vector x(N),b,S(N),work(N);
        gsl::matrix A,X(N,N),V(N,N);
        b.get(bf);
        A.get(Af);
        gsl_linalg_SV_decomp_mod(A,X,V,S,work);
        gsl_linalg_SV_solve(A,V,S,b,x);
        x.put(xf);
#else
        throw "least_square AdaBoost unimplemented (define HAVE_GSL if you want this)";
#endif
    }

    struct AdaBoost : virtual IBatchDense {
        narray< autodel<IModel> > models;
        floatarray alphas;
        floatarray werrs;

        AdaBoost() {
            pdef("do_reweighting",1,"reweight boosting parameters using LSQ");
            pdef("rounds",4,"number of rounds in AdaBoost");
            pdef("base_classifier","mlp","base classifier");
            pdef("save_intermediates",0,"save intermediate results");
        }

        int nfeatures() {
            return models[0]->nfeatures();
        }
        int nclasses() {
            return models[0]->nclasses();
        }
        const char *name() {
            return "adaboost";
        }
        void info(int depth,FILE *stream) {
            iprintf(stream,depth,"AdaBoost %d models\n",models.length());
            pprint(stream,depth);
            for(int i=0;i<models.length();i++) {
                iprintf(stream,depth,"alpha %g werr %g\n",alphas(i),werrs(i));
                models[i]->info(depth+1,stream);
            }
        }
        int nmodels() {
            return models.length();
        }
        void setModel(IModel *cf,int which) {
            models[which] = cf;
        }
        IModel &getModel(int i) {
            return *models[i];
        }

        void save(FILE *stream) {
            magic_write(stream,"adaboost");
            psave(stream);
            narray_write(stream,alphas);
            narray_write(stream,werrs);
            for(int i=0;i<models.length();i++)
                save_component(stream,models[i].ptr());
        }

        void load(FILE *stream) {
            magic_read(stream,"adaboost");
            pload(stream);
            narray_read(stream,alphas);
            narray_read(stream,werrs);
            models.resize(alphas.length());
            for(int i=0;i<models.length();i++) {
                IModel *loaded_model = dynamic_cast<IModel*>(load_component(stream));
                CHECK(loaded_model!=0);
                models[i] = loaded_model;
            }
        }

        narray<strbuf> names;
        narray<double> values;

        void pset(const char *name,double value) {
            if(pexists(name) || name[0]=='%') {
                this->IComponent::pset(name,value);
                return;
            }
            names.push() = name;
            values.push() = value;
            for(int i=0;i<nmodels();i++)
                if(models[i]->pexists(name))
                    models[i]->pset(name,value);
        }

        void setStored(IModel &dest) {
            for(int i=0;i<names.length();i++)
                dest.pset((char*)names(i),values(i));
        }

        IModel *make_Model() {
            IModel *result = make_model(pget("base_classifier"));
            setStored(*result);
            return result;
        }

        void reweight(IDataset &ds) {
            floatarray A(ds.nsamples(),nclasses(),models.length());
            floatarray b(ds.nsamples(),nclasses());
            debugf("info","starting prediction for reweighting\n");
#pragma omp parallel for
            for(int sample=0;sample<ds.nsamples();sample++) {
                floatarray v;
                floatarray p;
                ds.input1d(v,sample);
                int cls = ds.cls(sample);
                for(int k=0;k<models.length();k++) {
                    OutputVector ov;
                    models(k)->xoutputs(ov,v);
                    ov.as_array(v);
                    for(int r=0;r<nclasses();r++) {
                        b(sample,r) = (r==cls);
                        A(sample,r,k) = p(r);
                    }
                }
            }
            A.reshape(A.dim(0)*A.dim(1),A.dim(2));
            b.reshape(b.dim(0)*b.dim(1));
            debugf("info","starting least square\n");
            floatarray x;
            least_square(x,A,b);
            debugf("info","done least square\n");
            if(debug("info")) {
                printf("a = ");
                for(int i=0;i<alphas.length();i++)
                    printf(" %g",alphas(i));
                printf("\n");
                printf("x = ");
                for(int i=0;i<x.length();i++)
                    printf(" %g",x(i));
                printf("\n");
            }
            alphas = x;
        }

        void train_dense(IDataset &ds) {
            int nrounds = pgetf("rounds");
            int n = ds.nsamples();
            int nclass = ds.nclasses();
            pset("%nsamples",n);
            pset("%nclasses",nclass);
            floatarray weights(n);
            weights = 1.0;
            weights /= sum(weights);
            for(int round=0;round<nrounds;round++) {
                autodel<IModel> net(make_Model());
                // net->trainWeighted(data,classes,weights);

                // The perplexity is roughly the "number of samples"
                // in the weights distribution
                float perp = perplexity(weights);

                // FIXME cross-validation broken for AdaBoost
                // FIXME crude capacity control to avoid overtraining
                int maxhidden = max(net->pgetf("hidden_min")+5,max(20,int(perp/30.0)));
                net->pset("hidden_max",maxhidden);
                net->pset("hidden_hi",min(100,maxhidden));
                // net->set("crossvalidate",0);

                // Subsample 10x the perplexity; more doesn't really
                // make sense.
                int nsubsample = int(perp*10);
                nsubsample = max(1000,min(weights.length(),nsubsample));
                intarray subsample;
                weighted_sample(subsample,weights,nsubsample);
                Datasubset sds(ds,subsample);
                net->xtrain(sds);
                intarray predicted(n);
                floatarray v;
                for(int i=0;i<n;i++) {
                    ds.input1d(v,i);
                    predicted(i) = net->classify(v);
                }
                int err = 0;
                double werr = 0.0;
                for(int i=0;i<n;i++) {
                    if(predicted(i)!=ds.cls(i)) {
                        err++;
                        werr += weights(i);
                    }
                }
                werr /= sum(weights);
                debugf("info",
                        "AdaBoost round %d nsub %d errs %d werr %g perp %g\n",
                        round,nsubsample,err,werr,perp);
                double alpha = log((1.0-werr)/werr) + log(nclass-1.0);
                double exp_alpha = exp(alpha);
                for(int i=0;i<n;i++)
                    if(predicted(i)!=ds.cls(i))
                        weights(i) *= exp_alpha;
                weights /= sum(weights);
                models.push() = net.move();
                alphas.push() = alpha;
                werrs.push() = werr;
                int errs;
                if(1) {
                    int n = min(1000,ds.nsamples());
                    errs=0;
                    floatarray v;
                    for(int i=0;i<n;i++) {
                        int cls = ds.cls(i);
                        ds.input1d(v,i);
                        if(classify(v)!=cls) errs++;
                    }
                    debugf("train",
                            "AdaBoost error estimate: errs %g\n",
                           errs/float(n));
                    pset("%error_posterior",errs/float(n));
                }
                if(current_recognizer_ && pgetf("save_intermediates")) {
                    char buf[1000];
                    sprintf(buf,"_adaboost%06dd_round%02d.model",
                            getpid(),round);
                    current_recognizer_->save(stdio(buf,"w"));
                    debugf("info","AdaBoost saved %s\n",buf);
                }
            }
            if(pgetf("do_reweighting")) {
                reweight(ds);
            }
        }

        float outputs_dense(floatarray &result,floatarray &v) {
            result.resize(nclasses());
            result = 0;
            OutputVector p;
            for(int round=0;round<models.length();round++) {
                models(round)->xoutputs(p,v);
                double alpha = alphas(round);
                for(int i=0;i<p.length();i++)
                    result(i) += alpha * p(i);
            }
            result += min(result);
            result /= sum(result);
            return 0.0;
        }
    };

    ////////////////////////////////////////////////////////////////
    // CascadedMLP is similar to cascade correlation, but
    // doesn't train one unit at a time
    ////////////////////////////////////////////////////////////////

    struct CascadedMLP : virtual IBatchDense {
        narray< autodel<IModel> > models;

        CascadedMLP() {
            pdef("rounds",2,"number of cascaded networks");
            pdef("lrounds",999,"number of rounds to use during classification");
        }

        int nfeatures() {
            return models[0]->nfeatures();
        }
        int nclasses() {
            return models[0]->nclasses();
        }
        const char *name() {
            return "cmlp";
        }
        void info(int depth,FILE *stream) {
            iprintf(stream,depth,"CascadedMLP %d models\n",models.length());
            pprint(stream,depth);
            for(int i=0;i<models.length();i++) {
                iprintf(stream,depth+1,"CASCADED MODEL %d\n",i);
                models[i]->info(depth+1,stream);
            }
        }

        void save(FILE *stream) {
            magic_write(stream,"cascaded");
            psave(stream);
            scalar_write(stream,int(models.length()));
            for(int i=0;i<models.length();i++)
                save_component(stream,models[i].ptr());
        }

        void load(FILE *stream) {
            magic_read(stream,"cascaded");
            pload(stream);
            int nmodels;
            scalar_read(stream,nmodels);
            models.resize(nmodels);
            for(int i=0;i<models.length();i++) {
                IModel *loaded_model = dynamic_cast<IModel*>(load_component(stream));
                CHECK(loaded_model!=0);
                models[i] = loaded_model;
            }
        }

        IModel *make_Model() {
            return make_model("mappedmlp");
        }

        void train_dense(IDataset &ds) {
            int nrounds = pgetf("rounds");
            AugmentedDataset ads(ds);
            for(int round=0;round<nrounds;round++) {
                autodel<IModel> net(make_Model());
                net->xtrain(ads);
                int errs = 0;
#pragma omp parallel for
                for(int i=0;i<ds.nsamples();i++) {
                    floatarray v;
                    floatarray p;
                    ads.input1d(v,i);
                    OutputVector ov;
                    net->xoutputs(ov,v);
                    ov.as_array(p);
                    if(argmax(p)!=ds.cls(i))
                        errs++;
                    ads.augment(i,p);
                }
                models.push() = net;
                debugf("info","CascadedMLP round=%d errs=%d rate=%g\n",
                       round,errs,errs/float(ds.nsamples()));
            }
        }

        float outputs_dense(floatarray &result,floatarray &v) {
            int lrounds = pgetf("lrounds");
            result.resize(nclasses());
            result = 0;
            floatarray a;
            a = v;
            OutputVector out;
            floatarray p;
            for(int round=0;round<lrounds && round<models.length();round++) {
                if(round>0) a.append(p);
                OutputVector ov;
                models(round)->xoutputs(ov,a);
                ov.as_array(p);
            }
            result = p;
            result /= sum(result);
            return 0.0;
        }
    };

    ////////////////////////////////////////////////////////////////
    // classifier specifically intended for Latin script
    // (although there are actually not a lot of
    // customizations for latin script)
    ////////////////////////////////////////////////////////////////

    struct LatinClassifier : virtual IBatch {
        autodel<IModel> junkclass;
        autodel<IModel> charclass;
        autodel<IModel> ulclass;
        int junkchar;

        LatinClassifier() {
            pdef("junkchar",'~',"junk character");
            pdef("junkclass","mlp","junk classifier");
            pdef("charclass","mappedmlp","character classifier");
            pdef("junk",1,"train a separate junk classifier");
            pdef("ul",0,"do upper/lower reclassification");
            pdef("ulclass","mlp","upper/lower classifier");
            junkchar = -1;
            persist(junkclass,"junkclass");
            persist(charclass,"charclass");
            persist(ulclass,"ulclass");

        }
        int jc() {
            if(junkchar<0) junkchar = int(pgetf("junkchar"));
            return junkchar;
        }
        int nfeatures() {
            if(charclass)
                return charclass->nfeatures();
            else
                return 0;
        }
        int nclasses() {
            if(charclass)
                return max(jc()+1,charclass->nclasses());
            else
                return 0;
        }
        const char *name() {
            return "latin";
        }
        void info(int depth,FILE *stream) {
            iprintf(stream,depth+1,"CHARCLASS MODEL\n");
            charclass->info(depth+1,stream);
            iprintf(stream,depth+1,"JUNKCLASS MODEL\n");
            junkclass->info(depth+1,stream);
            iprintf(stream,depth+1,"ULCLASS MODEL\n");
            ulclass->info(depth+1,stream);
        }
        int nmodels() {
            return 2;
        }
        void setModel(IModel *cf,int which) {
            if(which==0) charclass = cf;
            else if(which==1) ulclass = cf;
        }
        IModel &getModel(int which) {
            if(which==0) return *charclass;
            else if(which==1) return *ulclass;
            throw "oops";
        }

        void train(IDataset &ds) {
            if(!junkclass) make_component(junkclass,pget("junkclass"));
            if(!charclass) make_component(charclass,pget("charclass"));
            if(!ulclass) make_component(ulclass,pget("ulclass"));

            debugf("info","training content classifier\n");
            if(pgetf("junk") && junkclass) {
                intarray nonjunk;
                for(int i=0;i<ds.nsamples();i++)
                    if(ds.cls(i)!=jc())
                        nonjunk.push(i);
                Datasubset nonjunkds(ds,nonjunk);
                charclass->xtrain(nonjunkds);
            } else {
                charclass->xtrain(ds);
            }

            if(pgetf("junk") && junkclass) {
                debugf("info","training junk classifier\n");
                intarray isjunk;
                for(int i=0;i<ds.nsamples();i++)
                    isjunk.push((ds.cls(i)==jc()));
                MappedDataset junkds(ds,isjunk);
                junkclass->xtrain(junkds);
            }

            if(pgetf("ul") && ulclass) {
                throw "ulclass not implemented";
            }
        }

        float outputs(OutputVector &result,floatarray &v) {
            OutputVector ov;

            result.clear();
            charclass->xoutputs(result,v);
            CHECK(result.nkeys()>0);

            if(pgetf("junk") && junkclass) {
                result.normalize();
                OutputVector jv;
                junkclass->xoutputs(jv,v);
                floatarray junk;
                jv.as_array(junk);
                for(int i=0;i<result.nkeys();i++)
                    result.values(i) *= junk(0);
                result(jc()) = junk(1);
            }

            if(pgetf("ul") && ulclass) {
                throw "ulclass not implemented";
            }

            return 0.0;
        }
    };

    struct RaveledExtractor : virtual IExtractor {
        virtual const char *name() { return "raveledfe"; }
        void extract(narray<floatarray> &out,floatarray &in) {
            out.clear();
            out.push() = in;
        }
    };

    struct ScaledImageExtractor : virtual IExtractor {
        virtual const char *name() { return "scaledfe"; }
        ScaledImageExtractor() {
            pdef("csize",30,"taget image size");
            pdef("aa",0,"anti-aliasing");
            pdef("noupscale",1,"no upscaling");
        }
        void rescale(floatarray &v,floatarray &sub) {
            CHECK_ARG(sub.rank()==2);
            int csize = int(pgetf("csize"));
            float s = max(sub.dim(0),sub.dim(1))/float(csize);
            if(pgetf("noupscale") && s<1.0) s = 1.0;
            float sig = s * pgetf("aa");
            float dx = (csize*s-sub.dim(0))/2;
            float dy = (csize*s-sub.dim(1))/2;
            if(sig>1e-3) gauss2d(sub,sig,sig);
            v.resize(csize,csize);
            v = 0;
            for(int i=0;i<csize;i++) {
                for(int j=0;j<csize;j++) {
                    float x = i*s-dx;
                    float y = j*s-dy;
                    if(x<0||x>=sub.dim(0)) continue;
                    if(y<0||y>=sub.dim(1)) continue;
                    float value = bilin(sub,x,y);
                    v(i,j) = value;
                }
            }
            debugf("fe","%d %d (%g) -> %d %d (%g)\n",
                   sub.dim(0),sub.dim(1),max(sub),
                   v.dim(0),v.dim(1),max(v));
            dsection("fe");
            if(dactive()) {
                dshown(sub,"a");
                dshown(v,"b");
                dwait();
                fprintf(stderr,"ok\n");
            }
        }
        void extract(narray<floatarray> &out,floatarray &in) {
            out.clear();
            floatarray &image = out.push();
            rescale(image,in);
            image /= max(1.0,max(image));
        }
    };

#ifdef HAVE_SQLITE3
#include <sqlite3.h>

    struct SqliteDataset : IDataset {
        sqlite3 *db;
        int n;
        int nc;
        int nf;

        SqliteDataset() {
            pdef("table","chars","table name for characters");
            pdef("file","dataset.sqlite3","location for dataset");
            pdef("sync",0,"commit after each insert");
            n = 0;
            nc = 0;
            nf = -1;
            db = 0;
        }
        ~SqliteDataset() {
            close();
        }
        const char *name() {
            return "sqliteds";
        }
        void save(FILE *stream) {
        }
        void load(FILE *stream) {
        }
        void open(const char *file) {
            debugf("info","binding to database %s\n",file);
            int status = sqlite3_open_v2(file,&db,
                                         SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE,0);
            if(status!=SQLITE_OK) throwf("%s: cannot open",file);
            // try to create the table (silently fails if it already
            // exists or if there is an error)
            const char *schema =
                "create table %s ("
                "id integer primary key,"
                "image blob,"
                "cls text,"
                "cluster integer,"
                "count integer,"
                "classes text,"
                "key text,"
                "pred text,"
                "cost real,"
                "style text,"
                "which text"
                ")";
            char schema_buf[10000];
            sprintf(schema_buf,schema,pget("table"));
            if(sqlite3_exec(db,schema_buf,0,0,0)!=SQLITE_OK) {
                debugf("info","create table: %s\n",sqlite3_errmsg(db));
            }
            sqlite3_exec(db,"PRAGMA synchronous=off",0,0,0);
        }
        void close() {
            sqlite3_close(db);
            db = 0;
        }
        static void pickle(bytearray &v,floatarray &image) {
            int w = image.dim(0);
            int h = image.dim(1);
            floatarray temp(h,w);
            float m = max(image);
            for(int i=0;i<w;i++)
                for(int j=0;j<h;j++)
                    temp(h-j-1,i) = image(i,j)*255.0/m;
            v.clear();
            v.push(h);
            v.push(w);
            for(int i=0;i<temp.length();i++)
                v.push(int(temp[i]));
        }
        static void unpickle(floatarray &image,bytearray &v) {
            int h = v[0];
            int w = v[1];
            image.resize(w,h);
            for(int j=0;j<h;j++)
                for(int i=0;i<w;i++)
                    image(i,j) = v[2+(h-j-1)*w+i]/255.0;
        }
        void add(const char *style,const char *which,floatarray &v,int c,
                 int pred,float cost) {
            CHECK(v.rank()==2);
            CHECK(v.dim(0)>0);
            CHECK(v.dim(0)<256);
            CHECK(v.dim(1)>0);
            CHECK(v.dim(1)<256);
            if(!db) open(pget("file"));
            const char *cmd = "insert into %s (style,which,image,cls,pred,cost) "
                "values (?,?,?,?,?,?)";
            char cmd_buf[1024];
            sprintf(cmd_buf,cmd,pget("table"));
            sqlite3_stmt *stmt;
#define SQLCHECK(S) \
    if((S)!=SQLITE_OK) { const char *err = sqlite3_errmsg(db); throwf("sqlite error: %s\n",err); }
            SQLCHECK(sqlite3_prepare(db,cmd_buf,-1,&stmt,0));
            // style, which
            SQLCHECK(sqlite3_bind_text(stmt,1,style,strlen(style),SQLITE_TRANSIENT));
            SQLCHECK(sqlite3_bind_text(stmt,2,which,strlen(which),SQLITE_TRANSIENT));
            // image
            bytearray bv;
            pickle(bv,v);
            CHECK(bv.length()==v.length()+2);
            SQLCHECK(sqlite3_bind_blob(stmt,3,&bv(0),bv.length(),SQLITE_TRANSIENT));
            // cls, pred
            char text[2];
            text[0] = c; text[1] = 0;
            SQLCHECK(sqlite3_bind_text(stmt,4,text,strlen(text),SQLITE_TRANSIENT));
            text[0] = pred; text[1] = 0;
            SQLCHECK(sqlite3_bind_text(stmt,5,text,strlen(text),SQLITE_TRANSIENT));
            // cost
            SQLCHECK(sqlite3_bind_double(stmt,6,cost));
            CHECK(sqlite3_step(stmt)==SQLITE_DONE);
            SQLCHECK(sqlite3_finalize(stmt));
            if(pgetf("sync")) {
                char *err = 0;
                if(sqlite3_exec(db,"COMMIT",0,0,&err)!=SQLITE_OK)
                    throwf("%s:COMMIT",err);
            }
            debugf("debug","inserted\n");
            n++;
            if(c>=nc) nc = c+1;
            if(nf<0) nf = v.length();
        }
        void add(floatarray &v,int c) {
            add("","",v,c,-1,-1);
        }
        int nsamples() {
            return n;
        }
        int nclasses() {
            return nc;
        }
        int nfeatures() {
            return nf;
        }
        void clear() {
            throw "unimplemented";
        }
        int cls(int i) {
            throw "unimplemented";
        }
        void input(floatarray &v,int i) {
            throw "unimplemented";
        }
        int id(int i) {
            throw "unimplemented";
        }
    };

    struct SqliteBuffer : IModel {
        int nf,nc,n;
        autodel<SqliteDataset> ds;
        SqliteBuffer() {
            pdef("file","data.sqlite3","database file name");
            n = 0;
            nf = -1;
            nc = -1;
        }
        const char *name() {
            return "sqlitebuffer";
        }
        void info(int depth,FILE *stream) {
            iprintf(stream,depth,"SqliteBuffer\n");
            pprint(stream,depth);
        }
        void train(IDataset &ds) {
            floatarray v;
            for(int i=0;i<ds.nsamples();i++) {
                int c = ds.cls(i);
                ds.input(v,i);
                add(v,c);
            }
        }
        void add(floatarray &v,int c) {
            if(!ds) {
                ds = new SqliteDataset();
                ds->open(pget("file"));
            }
            debugf("debug","adding %dx%d [%g,%g] %d\n",v.dim(0),v.dim(1),min(v),max(v),c);
            if(c>=nc) nc = c+1;
            nf = v.length();
            ds->add(v,c);
            n++;
        }
        void updateModel() {
        }
        // ignore these
        void save(FILE *stream) {
            debugf("warn","%s: ignoring save; data in %s",name(),pget("file"));
        }
        void load(FILE *stream) {
            debugf("warn","%s: ignoring load; data in %s",name(),pget("file"));
        }
        // we can't classify
        float outputs(OutputVector &z,floatarray &x) {
            throw Unimplemented();
        }
    };
#endif


    inline float sqr(float x) { return x*x; }

    struct EuclideanDistances : IDistComp {
        intarray counts_;
        narray<floatarray> vectors;
        virtual const char *name() { return "edist"; }
        virtual void add(floatarray &obj) {
            vectors.push() = obj;
            counts_.push() = 1;
        }
        virtual void distances(floatarray &ds,floatarray &obj) {
            int n = obj.length();
            ds.resize(n);
            ds = 1e38;
#pragma omp parallel for
            for(int i=0;i<vectors.length();i++) {
                double total = 0.0;
                floatarray &v = vectors[i];
                if(v.length()!=n) continue;
                float *p = &obj[0];
                float *q = &v[0];
                for(int j=0;j<n;j++) total += sqr(p[j]-q[j]);
                ds(i) = total;
            }
        }
        virtual void merge(int i,floatarray &obj,float weight) {
            CHECK_ARG(weight>=0.0 && weight<=1.0);
            CHECK_ARG(vectors(i).length()==obj.length());
            int n = obj.length();
            float cweight = 1.0-weight;
            floatarray &v = vectors(i);
            for(int j=0;j<n;j++) v(i) = cweight*v(i) + weight*obj(i);
            counts_(i)++;
        }
        virtual int counts(int i) {
            return counts_(i);
        }
        virtual void vector(floatarray &v,int i) {
            v = vectors(i);
        }
        virtual floatarray &vector(int i) {
            return vectors(i);
        }
    };


    // train multiple classifiers and average (FIXME: implement with resampling etc.)

    void init_glclass() {
        static bool init = false;
        if(init) return;
        init = true;

        // base classifiers
        component_register<KnnClassifier>("knn");
        component_register<EnetClassifier>("enet");
        component_register<AutoMlpClassifier>("mlp");
        component_register<AutoMlpClassifier>("mappedmlp");

        // classifier combination
        component_register<CascadedMLP>("cmlp");
        component_register<AdaBoost>("adaboost");
        component_register<LatinClassifier>("latin");

        // feature extractors
        component_register<RaveledExtractor>("raveledfe");
        component_register<ScaledImageExtractor>("scaledfe");

        // distance components
        component_register<EuclideanDistances>("edist");

        // other components
        typedef RowDataset<float8> RowDataset8;
        typedef RaggedDataset<float8> RaggedDataset8;
        component_register<RowDataset8>("rowdataset8");
        component_register<RaggedDataset8>("raggeddataset8");
        component_register<SqliteDataset>("sqliteds");
        component_register<SqliteBuffer>("sqlitebuffer");

        extern void init_glbits(),init_glcuts();
        init_glbits();
        init_glcuts();
    }

    IRecognizeLine *current_recognizer_ = 0;
}
