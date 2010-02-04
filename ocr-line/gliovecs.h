// -*- C++ -*-

#ifndef gliovecs_h__
#define gliovecs_h__

#include <typeinfo>
#include <stdio.h>
#include <colib/narray.h>
#include "colib/narray-binio.h"

namespace glinerec {

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
        float &value(int i) {
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
        float sum() {
            double total;
            for(int i=0;i<values.length();i++)
                total += values[i];
            return total;
        }
        void normalize() {
            using namespace narray_ops;
            values /= sum();
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
