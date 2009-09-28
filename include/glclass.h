// -*- C++ -*-

#ifndef glclass_h__
#define glclass_h__

#include <typeinfo>
#include <stdio.h>
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

    // Input vectors to classifiers are collections of narrays.
    // Many classifiers view them as just flat vectors, but
    // some classifiers may require additional structure.

    struct InputVector {
        narray<strg> names;
        narray<floatarray> inputs;
        floatarray flat;

        InputVector() {
        }
        InputVector(floatarray &v) {
            append(v,"_");
        }

        int length() {
            int total = 0;
            for(int i=0;i<inputs.length();i++)
                total += inputs[i].length();
            return total;
        }
        int dim(int i) {
            CHECK_ARG(i==0);
            return length();
        }
        void clear() {
            inputs.clear();
            names.clear();
            flat.clear();
        }
        void operator=(floatarray &v) {
            clear();
            inputs.push() = v;
            names.push("_");
        }
        void append(floatarray &v,const char *name) {
            flat.clear();
            inputs.push() = v;
            names.push() = name;
        }

        // access the chunks directly

        int nchunks() {
            return inputs.length();
        }
        floatarray &chunk(int i) {
            return inputs[i];
        }
        const char *name(int i) {
            return names[i].c_str();
        }
        floatarray &chunk(const char *name) {
            for(int i=0;i<names.length();i++) {
                if(names[i]==name)
                    return inputs[i];
            }
            throwf("%s: no such input chunk",name);
        }

        // vector-like access

        floatarray &ravel() {
            flat.clear();
            for(int i=0;i<inputs.length();i++)
                flat.append(inputs[i]);
            return flat;
        }
        void ravel(floatarray &v) {
            v.clear();
            for(int i=0;i<inputs.length();i++)
                v.append(inputs[i]);
        }
#if 0
        float &operator()(int index) {
            return flat[index];
        }
        float &operator[](int index) {
            return flat[index];
        }
#endif
    };


    // OutputVector is a sparse vector class, used for representing
    // classifier outputs.

    struct OutputVector {
        int len;
        intarray keys;
        floatarray values;
        floatarray *result;
        OutputVector() {
            result = 0;
        }
        OutputVector(int n) {
            init(n);
            len = 0;
            result = 0;
        }

        // If it's initialized with an array, the result vector
        // is copied into that array when the vector gets destroyed.
        // This allows calls like classifier->outputs(v,x); with
        // floatarray v.

        OutputVector(floatarray &v) {
            result = &v;
            v.clear();
        }
        ~OutputVector() {
            if(result) as_array(*result);
        }

        // Sparse vector access.

        void clear() {
            keys.clear();
            values.clear();
        }
        int nkeys() {
            return keys.length();
        }
        int key(int i) {
            return keys[i];
        }
        float value(int i) {
            return values[i];
        }

        // Dense vector conversions and access.

        void init(int n=0) {
            keys.resize(n);
            for(int i=0;i<n;i++) keys[i] = i;
            values.resize(n);
            values = 0;
        }
        void copy(floatarray &v,float eps=1e-11) {
            clear();
            int n = v.length();
            for(int i=0;i<n;i++) {
                float value = v[i];
                if(fabs(value)>=eps) {
                    keys.push(i);
                    values.push(value);
                }
            }
            len = v.length();
            keys.resize(len);
            for(int i=0;i<len;i++) keys[i] = i;
            values = v;
        }
        void operator=(floatarray &v) {
            copy(v);
        }
        int length() {
            return len;
        }
        int dim(int i) {
            CHECK_ARG(i==0);
            return len;
        }
        float &operator()(int index) {
            for(int j=0;j<keys.length();j++)
                if(keys[j]==index) return values[j];
            keys.push(index);
            values.push(0);
            if(index>=len) len = index+1;
            return values.last();
        }
        float &operator[](int i) {
            return operator()(i);
        }
        floatarray as_array() {
            floatarray result;
            result.resize(length());
            result = 0;
            for(int i=0;i<keys.length();i++)
                result[keys[i]] = values[i];
            return result;
        }
        void as_array(floatarray &result) {
            result.resize(length());
            result = 0;
            for(int i=0;i<keys.length();i++)
                result[keys[i]] = values[i];
        }

        // Some common operators.

        void operator/=(float value) {
            using namespace narray_ops;
            values /= value;
        }
        int argmax() {
            int index = iulib::argmax(values);
            return keys[index];
        }
        float max() {
            return iulib::max(values);
        }
    };

    inline float sum(OutputVector &v) {
        return iulib::sum(v.values);
    }

    inline void ctranslate_vec(OutputVector &v,intarray &translation) {
        for(int i=0;i<v.keys.length();i++)
            v.keys[i] = translation[v.keys[i]];
        v.len = max(v.keys)+1;
    }

    struct IModel : IComponent {
        IModel() {
            pdef("cds","rowdataset8","default dataset buffer class");
        }

        virtual const char *name() { return "IModel"; }
        virtual const char *interface() { return "IModel"; }

        // inquiry functions
        virtual int nfeatures() {throw Unimplemented();}
        virtual int nclasses() {throw Unimplemented();}
        virtual float complexity() {return 1.0;}

        // submodels
        virtual int nmodels() { return 0; }
        virtual void setModel(IModel *,int i) { throw "no submodels"; }
        virtual IComponent &getModel(int i) { throw "no submodels"; }

        // update this model in place
        virtual void copy(IModel &) { throw Unimplemented(); }

        float outputs(OutputVector &result,InputVector &v) {
            return outputs_impl(result,v);
        }
        float outputs(floatarray &result,floatarray &v) {
            OutputVector result_(result);
            InputVector v_(v);
            return outputs_impl(result_,v_);
        }

        virtual float outputs_impl(OutputVector &result,InputVector &v) {
            floatarray result_;
            floatarray v_;
            v.ravel(v_);
            float value = outputs_impl(result_,v_);
            result.copy(result_);
            return value;
        }
        virtual float outputs_impl(floatarray &result,floatarray &v) {
            throw Unimplemented();
        }
    public:
#if 0
        float outputs(OutputVector &result,floatarray &v) {
            InputVector v_;
            return outputs(result,v_);
        }
        float outputs(floatarray &result,InputVector &v) {
            OutputVector result_(result);
            return outputs(result_,v);
        }
        float outputs(floatarray &result,floatarray &v) {
            OutputVector result_(result);
            InputVector v_(v);
            return outputs(result_,v_);
        }
#endif

        virtual float cost(floatarray &v) {
            OutputVector temp;
            InputVector v_(v);
            return outputs(temp,v_);
        }

        // convenience function
        virtual int classify(floatarray &v) {
            OutputVector p;
            InputVector v_(v);
            outputs(p,v_);
            return p.argmax();
        }

        // estimate the cross validated error from the training data seen
        virtual float crossValidatedError() { throw Unimplemented(); }

        // batch training with dataset interface
        virtual void train(IDataset &dataset) = 0;

        // incremental training & default implementation in terms of batch
        autodel<IExtDataset> ds;
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
    };

    ////////////////////////////////////////////////////////////////
    // MappedClassifier remaps classes prior to classification.
    ////////////////////////////////////////////////////////////////

    struct MappedClassifier : IModel {
        autodel<IModel> cf;
        intarray c2i,i2c;
        MappedClassifier() {
        }
        MappedClassifier(IModel *cf):cf(cf) {
        }
        void info(int depth,FILE *stream) {
            iprintf(stream,depth,"MappedClassifier\n");
            pprint(stream,depth);
            iprintf(stream,depth,"mapping from %d to %d dimensions\n",
                   c2i.dim(0),i2c.dim(0));
            if(!!cf) cf->info(depth+1);
        }
        int nmodels() {
            return 1;
        }
        void setModel(IModel *cf,int which) {
            this->cf = cf;
        }
        IModel &getModel(int i) {
            return *cf;
        }
        int nfeatures() {
            return cf->nfeatures();
        }
        int nclasses() {
            return c2i.length();
        }
        const char *name() {
            return "mapped";
        }
        void save(FILE *stream) {
            magic_write(stream,"mapped");
            psave(stream);
            narray_write(stream,c2i);
            narray_write(stream,i2c);
            // cf->save(stream);
            save_component(stream,cf.ptr());
        }

        void load(FILE *stream) {
            magic_read(stream,"mapped");
            pload(stream);
            narray_read(stream,c2i);
            narray_read(stream,i2c);
            // cf->load(stream);
            cf = dynamic_cast<IModel*>(load_component(stream));
            CHECK_ARG(!!cf);
        }

        void pset(const char *name,const char *value) {
            if(pexists(name)) pset(name,value);
            if(cf->pexists(name)) cf->pset(name,value);
        }
        void pset(const char *name,double value) {
            if(pexists(name)) pset(name,value);
            if(cf->pexists(name)) cf->pset(name,value);
        }
        int classify(floatarray &x) {
            return i2c(cf->classify(x));
        }
        float outputs_impl(OutputVector &z,InputVector &x) {
            float result = cf->outputs(z,x);
            ctranslate_vec(z,i2c);
            return result;
        }

        static void hist(intarray &h,intarray &a) {
            h.resize(max(a)+1);
            h = 0;
            for(int i=0;i<a.length();i++)
                h(a(i))++;
        }

        struct MappedDataset : IDataset {
            IDataset &ds;
            intarray &c2i;
            int nc;
            const char *name() { return "mappeddataset"; }
            MappedDataset(IDataset &ds,intarray &c2i) : ds(ds),c2i(c2i) {
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
            MappedDataset mds(ds,c2i);
            cf->train(mds);
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
