// -*- C++ -*-

#ifndef gliovecs_h__
#define gliovecs_h__

#include <typeinfo>
#include <stdio.h>
#include <colib/narray.h>
#include "narray-binio.h"

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
        void operator=(InputVector &v) {
            clear();
            for(int i=0;i<v.names.length();i++) {
                inputs.push() = v.inputs[i];
                names.push() = v.names[i];
            }
        }
        void makelike(InputVector &v) {
            *this = v;
        }
        void checklike(InputVector &v) {
            CHECK(v.nchunks()==nchunks());
            for(int i=0;i<v.nchunks();i++) {
                CHECK(inputs[i].length()==v.inputs[i].length());
                // samedims
            }
        }
        void fillwith(floatarray &v) {
            int i = 0;
            while(i<v.length()) {
                for(int k=0;k<inputs.length();k++) {
                    for(int j=0;j<inputs[k].length();j++) {
                        if(i>=v.length()) throw "InputVector larger than floatarray source";
                        inputs[k][j] = v[i++];
                    }
                }
            }
            if(i!=v.length()) throw "InputVector smaller than floatarray source";
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

}

#endif
