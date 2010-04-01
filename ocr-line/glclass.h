// -*- C++ -*-

#ifndef glclass_h__
#define glclass_h__

#include <typeinfo>
#include <stdio.h>
#include "gliovecs.h"
#include "gldataset.h"

namespace {
    // compute a classmap that maps a set of possibly sparse classes onto a dense
    // list of new classes and vice versa

    void classmap(intarray &class_to_index,intarray &index_to_class,intarray &classes) {
        int nclasses = max(classes)+1;
        intarray hist(nclasses);
        hist.fill(0);
        for(int i=0;i<classes.length();i++) {
            if(classes(i)==-1) continue;
            hist(classes(i))++;
        }
        int count = 0;
        for(int i=0;i<hist.length();i++)
            if(hist(i)>0) count++;
        class_to_index.resize(nclasses);
        class_to_index.fill(-1);
        index_to_class.resize(count);
        index_to_class.fill(-1);
        int index=0;
        for(int i=0;i<hist.length();i++) {
            if(hist(i)>0) {
                class_to_index(i) = index;
                index_to_class(index) = i;
                index++;
            }
        }
        CHECK(class_to_index.length()==nclasses);
        CHECK(index_to_class.length()==max(class_to_index)+1);
        CHECK(index_to_class.length()<=class_to_index.length());
    }

    // unpack posteriors or discriminant values using a translation map
    // this is to go from the output of a classifier with a limited set
    // of classes to a full output vector

    void ctranslate_vec(floatarray &result,floatarray &input,intarray &translation) {
        result.resize(max(translation)+1);
        result.fill(0);
        for(int i=0;i<input.length();i++)
            result(translation(i)) = input(i);
    }

    void ctranslate_vec(floatarray &v,intarray &translation) {
        floatarray temp;
        ctranslate_vec(temp,v,translation);
        v.move(temp);
    }

    // translate classes using a translation map

    void ctranslate(intarray &values,intarray &translation) {
        for(int i=0;i<values.length();i++)
            values(i) = translation(values(i));
    }

    void ctranslate(intarray &result,intarray &values,intarray &translation) {
        result.resize(values.length());
        for(int i=0;i<values.length();i++) {
            int v = values(i);
            if(v<0) result(i) = v;
            else result(i) = translation(v);
        }
    }

    // count the number of distinct classes in a list of classes (also
    // works for a translation map

    int classcount(intarray &classes) {
        int nclasses = max(classes)+1;
        intarray hist(nclasses);
        hist.fill(0);
        for(int i=0;i<classes.length();i++) {
            if(classes(i)==-1) continue;
            hist(classes(i))++;
        }
        int count = 0;
        for(int i=0;i<hist.length();i++)
            if(hist(i)>0) count++;
        return count;
    }

}

namespace glinerec {

    struct IExtractor : virtual IComponent {
        virtual const char *name() { return "IExtractor"; }
        virtual const char *interface() { return "IExtractor"; }
        virtual void extract(narray<floatarray> &out,floatarray &in) {
            throw Unimplemented();
        }
        virtual void extract(floatarray &out,floatarray &in) {
            out.clear();
            narray<floatarray> items;
            extract(items,in);
            for(int i=0;i<items.length();i++) {
                floatarray &a = items[i];
                for(int j=0;j<a.length();j++)
                    out.push(a[j]);
            }
        }
        virtual void extract(bytearray &out,bytearray &in) {
            floatarray fin,fout;
            fin = in;
            extract(fout,fin);
            out = fout;
        }
        // convenience methods
        void extract(floatarray &v) {
            floatarray temp;
            extract(temp,v);
            v.move(temp);
        }
    };

    struct ExtractedDataset : virtual IDataset {
        IDataset &ds;
        IExtractor &ex;
        ExtractedDataset(IDataset &ds,IExtractor &ex)
            : ds(ds),ex(ex) {}
        int nsamples() { return ds.nsamples(); }
        int nclasses() { return ds.nclasses(); }
        int nfeatures() { return ds.nfeatures(); }
        void input(floatarray &v,int i) {
            floatarray temp;
            ds.input(temp,i);
            ex.extract(v,temp);
        }
        int cls(int i) { return ds.cls(i); }
        int id(int i) { return ds.id(i); }
    };

    struct IModel : virtual IComponent {
        autodel<IExtractor> extractor;
        IModel() {
            persist(extractor,"extractor");
            pdef("extractor","none","feature extractor");
            setExtractor(pget("extractor"));
        }
        void setExtractor(const char *name) {
            if(name==0 || !strcmp("none",name)) {
                extractor = 0;
            } else {
                make_component(extractor,name);
            }
            pset("extractor",name);
        }
        IExtractor *getExtractor() {
            return extractor.ptr();
        }
        virtual const char *name() { return "IModel"; }
        virtual const char *interface() { return "IModel"; }
        void xadd(floatarray &v,int c) {
            floatarray temp;
            if(extractor) extractor->extract(temp,v);
            else temp = v;
            add(temp,c);
        }
        float xoutputs(OutputVector &ov,floatarray &v) {
            floatarray temp;
            if(extractor) extractor->extract(temp,v);
            else temp = v;
            return outputs(ov,temp);
        }

        void xtrain(IDataset &ds) {
            if(!extractor) {
                train(ds);
            } else {
                ExtractedDataset eds(ds,*extractor);
                train(eds);
            }
        }
        virtual void updateModel() {
            throw Unimplemented();
        }
    protected:
        virtual void add(floatarray &v,int c) {
            throw Unimplemented();
        }
        virtual float outputs(OutputVector &ov,floatarray &x) {
            throw Unimplemented();
        }
        virtual void train(IDataset &ds) {
            floatarray v;
            for(int i=0;i<ds.nsamples();i++) {
                ds.input(v,i);
                add(v,ds.cls(i));
            }
        }
    public:

        // special inquiry functions

        virtual int nclasses() { return -1; }
        virtual int nfeatures() { return -1; }
        virtual void copy(IModel &) { throw Unimplemented(); }
        virtual int nprotos() {return 0;}
        virtual void getproto(floatarray &v,int i,int variant) { throw Unimplemented(); }
        virtual int nmodels() { return 0; }
        virtual void setModel(IModel *,int i) { throw "no submodels"; }
        virtual IComponent &getModel(int i) { throw "no submodels"; }

        // convenience functions

        float outputs(floatarray &p,floatarray &x) {
            OutputVector ov;
            float cost = xoutputs(ov,x);
            ov.as_array(p);
            return cost;
        }
        int classify(floatarray &v) {
            OutputVector p;
            xoutputs(p,v);
            return p.argmax();
        }
    };

    struct OmpClassifier {
        autodel<IModel> model;
        narray<floatarray> inputs;
        narray<OutputVector> outputs;
        void clear() {
            inputs.clear();
            outputs.clear();
        }
        void classify() {
            int total = inputs.length();
#pragma omp parallel for
            for(int i=0;i<inputs.length();i++) {
                model->xoutputs(outputs[i],inputs[i]);
#pragma omp critical
                if(total--%1000==0) {
                    debugf("info","remaining %d\n",total);
                }
            }
        }
        int input(floatarray &a) {
            int result = inputs.length();
            inputs.push() = a;
            outputs.push();
            return result;
        }
        void output(OutputVector &ov,int i) {
            ov = outputs[i];
        }
        void load(const char *file) {
            model = dynamic_cast<IModel*>(load_component(stdio(file,"r")));
            CHECK(model!=0);
        }
    };

    struct IBatch : virtual IModel {
        virtual const char *name() { return "IBatch"; }
        virtual const char *interface() { return "IBatch"; }

        virtual void train(IDataset &dataset) {
            throw Unimplemented();
        }
        virtual float outputs(OutputVector &result,floatarray &v) {
            throw Unimplemented();
        }

        // incremental training for batch models

        autodel<IExtDataset> ds;

        IBatch() {
            pdef("cds","rowdataset8","default dataset buffer class");

        }

        virtual void add(floatarray &v,int c) {
            if(!ds) {
                debugf("info","allocating %s buffer for classifier\n",pget("cds"));
                make_component(pget("cds"),ds);
            }
            ds->add(v,c);
        }

        virtual void updateModel() {
            if(!ds) return;
            debugf("info","updateModel %d samples, %d features, %d classes\n",
                   ds->nsamples(),ds->nfeatures(),ds->nclasses());
            debugf("info","updateModel memory status %d Mbytes, %d Mvalues\n",
                   int((long long)sbrk(0)/1000000),
                   (ds->nsamples() * ds->nfeatures())/1000000);
            train(*ds);
            ds = 0;
        }
        virtual const char *command(const char **argv) {
            return IModel::command(argv);
        }
    };

    struct IBatchDense : virtual IBatch {
        intarray c2i,i2c;

        IBatchDense() {
            persist(c2i,"c2i");
            persist(i2c,"i2c");
        }

        virtual const char *command(const char **argv) {
            if(!strcmp(argv[0],"debug_map")) {
                for(int i=0;i<i2c.length();i++)
                    printf("%d -> %d\n",i,i2c[i]);
                return 0;
            }
            return IBatch::command(argv);
        }

        float outputs(OutputVector &result,floatarray &v) {
            floatarray out;
            float cost = outputs_dense(out,v);
            result.clear();
            for(int i=0;i<out.length();i++)
                result(i2c(i)) = out(i);
            return cost;
        }

        struct TranslatedDataset : virtual IDataset {
            IDataset &ds;
            intarray &c2i;
            int nc;
            const char *name() { return "mappeddataset"; }
            TranslatedDataset(IDataset &ds,intarray &c2i) : ds(ds),c2i(c2i) {
                nc = max(c2i)+1;
            }
            int nclasses() { return nc; }
            int nfeatures() { return ds.nfeatures(); }
            int nsamples() { return ds.nsamples(); }
            void input(floatarray &v,int i) { ds.input(v,i); }
            int cls(int i) { return c2i(ds.cls(i)); }
            int id(int i) { return ds.id(i); }
        };

        void train(IDataset &ds) {
            CHECK(ds.nsamples()>0);
            CHECK(ds.nfeatures()>0);
            if(c2i.length()<1) {
                intarray raw_classes;
                for(int i=0;i<ds.nsamples();i++)
                    raw_classes.push(ds.cls(i));
                classmap(c2i,i2c,raw_classes);
                intarray classes;
                ctranslate(classes,raw_classes,c2i);
                debugf("info","[mapped %d to %d classes]\n",c2i.length(),i2c.length());
            }
            TranslatedDataset mds(ds,c2i);
            train_dense(mds);
        }

        virtual void train_dense(IDataset &dataset) {
            throw Unimplemented();
        }
        virtual float outputs_dense(floatarray &result,floatarray &v) {
            throw Unimplemented();
        }
    };

    struct IDistComp : IComponent {
        virtual const char *name() { return "IDistComp"; }
        virtual const char *interface() { return "IDistComp"; }
        virtual void add(floatarray &obj) {
            throw Unimplemented();
        }
        virtual void distances(floatarray &ds,floatarray &obj) {
            throw Unimplemented();
        }
        virtual int find(floatarray &obj,float eps) {
            floatarray ds;
            distances(ds,obj);
            int index = argmin(ds);
            if(ds[index]<eps) return index;
            return -1;
        }
        virtual void merge(int i,floatarray &obj,float weight) {
            throw Unimplemented();
        }
        virtual int length() {
            throw Unimplemented();
        }
        virtual int counts(int i) {
            throw Unimplemented();
        }
        virtual floatarray &vector(int i) {
            throw Unimplemented();
        }
        virtual void vector(floatarray &v,int i) {
            throw Unimplemented();
        }
        int nearest(floatarray &obj) {
            floatarray ds;
            distances(ds,obj);
            return argmin(ds);
        }
    };

    void confusion_matrix(intarray &confusion,IModel &classifier,floatarray &data,intarray &classes);
    void confusion_matrix(intarray &confusion,IModel &classifier,IDataset &ds);
    void confusion_print(intarray &confusion);
    int compute_confusions(intarray &list,intarray &confusion);
    void print_confusion(IModel &classifier,floatarray &vs,intarray &cs);
    float confusion_error(intarray &confusion);

    void least_square(floatarray &xf,floatarray &Af,floatarray &bf);

    inline IModel *make_model(const char *name) {
        IModel *result = dynamic_cast<IModel*>(component_construct(name));
        CHECK(result!=0);
        return result;
    }

    extern IRecognizeLine *current_recognizer_;
}

#endif
