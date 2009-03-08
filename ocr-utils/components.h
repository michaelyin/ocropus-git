// -*- C++ -*-

#ifndef components_h__
#define components_h__

#include <typeinfo>
#include "colib/colib.h"
#include "iulib/iulib.h"

using namespace colib;
using namespace iulib;

#ifndef _OPENMP
#define THREAD 0
#define NTHREADS 1
#else
#include <omp.h>
#define THREAD omp_get_thread_num()
#define NTHREADS omp_get_num_threads()
#endif



namespace ocropus {

    extern const char *global_verbose_params;

    /// Base class for OCR components.

    struct IComponent {
	const char *verbose_pattern;

        IComponent() {
	    verbose_pattern = "%%%";
            checked = false;
            bool enabled = true;
#ifdef _OPENMP
            enabled = (omp_get_thread_num()==0);
#endif
            if(enabled || global_verbose_params) {
                if(global_verbose_params) {
                    verbose_pattern = global_verbose_params;
                } else if(getenv("verbose_params")) {
                    if(!strcmp(getenv("verbose_params"),"1"))
                        verbose_pattern = "";
                    else
                        verbose_pattern = getenv("verbose_params");
                }
	    }
        }

        /// object name
        virtual const char *name() {
            return typeid(*this).name();
        }

        /// brief description
        virtual const char *description() {
            return typeid(*this).name();
        }

        /// print brief description
        virtual void print() {
            printf("<%s (%s) %p>\n",name(),typeid(*this).name(),this);
        }

        /// misc information logged about the history of the component
        strbuf object_history;

        /// print longer info to stdout
        virtual void info(int depth=0,FILE *stream=stdout) {
            fprintf(stream,"%*s",depth,"");
            fprintf(stream,"%s\n",description());
            fprintf(stream,"%s\n",(const char *)object_history);
            pprint(stream,depth);
        }

        /// saving and loading (if implemented)
        virtual void save(FILE *stream) {throw Unimplemented();}
        virtual void load(FILE *stream) {throw Unimplemented();}
        virtual void save(const char *path) {save(stdio(path,"wb"));}
        virtual void load(const char *path) {load(stdio(path,"rb"));}

        /// parameter setting and loading
    private:
        strhash<strbuf> params;
        strhash<bool> shown;
        bool checked;
    public:
        /// verify that there are no extra parameters in the environment
        virtual void check_parameters_();

        // Define a string parameter for this component.  Parameters
        // should be defined once in the constructor, together with
        // a default value and a documentation string.
        // Names starting with a '%' are not parameters, but rather
        // information about the component computed while running
        // (it's saved along with the parameters when saving the
        // component).
        void pdef(const char *name,const char *value,const char *doc) {
            if(name[0]=='%') throwf("pdef: %s must not start with %%",name);
            if(params.find(name)) throwf("pdefs: %s: parameter already defined");
            params(name) = value;
            strbuf key;
            key = this->name();
            key += "_";
            key += name;
            bool altered = false;
            if(getenv(key.ptr())) {
                const char *evalue = getenv(key.ptr());
                if(strcmp(evalue,value)) altered = true;
                params(name) = evalue;
            }
            if(!shown.find(key.ptr())) {
                if(altered && !strcmp(verbose_pattern,"?")) {
                    fprintf(stderr,"param altered %s=%s # %s\n",
                            key.ptr(),params(name).ptr(),doc);
                } else if(strstr(key.ptr(),verbose_pattern)!=0) {
                    fprintf(stderr,"param default %s=%s # %s\n",
                            key.ptr(),params(name).ptr(),doc);
                }
                shown(key.ptr()) = true;
            }
        }
        void pdef(const char *name,double value,const char *doc) {
            strbuf svalue;
            svalue.format("%g",value);
            pdef(name,svalue.ptr(),doc);
        }
        void pdef(const char *name,int value,const char *doc) {
            pdef(name,double(value),doc);
        }
        void pdef(const char *name,bool value,const char *doc) {
            pdef(name,double(value),doc);
        }
        // Check whether the parameter exists.
        virtual bool pexists(const char *name) {
            return !!params.find(name);
        }
        // Set a parameter; this allows changing the parameter after it
        // has been defined.  It should be called by other parts of the
        // system if they want to change a parameter value.
        // These are virtual so that classes can forward them if necessary.
        virtual void pset(const char *name,const char *value) {
            if(name[0]!='%' && !params.find(name)) throwf("pset: %s: no such parameter",name);
            params(name) = value;
            if(strstr(name,verbose_pattern))
                fprintf(stderr,"set %s_%s=%s\n",this->name(),name,value);
        }
        virtual void pset(const char *name,double value) {
            strbuf svalue;
            svalue.format("%g",value);
            pset(name,svalue.ptr());
        }
        void pset(const char *name,int value) {
            pset(name,double(value));
        }
        void pset(const char *name,bool value) {
            pset(name,double(value));
        }
        // Get a string paramter.  This can be called both from within the class
        // implementation, as well as from external functions, in order to see
        // what current parameter settings are.
        const char *pget(const char *name) {
            if(!checked) check_parameters_();
            if(!params.find(name)) throwf("pget: %s: no such parameter",name);
            return params(name).ptr();
        }
        double pgetf(const char *name) {
            if(!checked) check_parameters_();
            double value;
            if(sscanf(pget(name),"%lg",&value)!=1)
                throwf("pgetf: %s=%s: bad number format",name,params(name).ptr());
            return value;
        }
        // Save the parameters to the string.  This should get called from save().
        // The format is binary and not necessarily fit for human consumption.
        void psave(FILE *stream) {
            narray<const char *> keys;
            params.keys(keys);
            for(int i=0;i<keys.length();i++) {
                fprintf(stream,"%s=%s\n",keys(i),params(keys(i)).ptr());
            }
            fprintf(stream,"END_OF_PARAMETERS=HERE\n");
        }
        // Load the parameters from the string.  This should get called from load().
        // The format is binary and not necessarily fit for human consumption.
        void pload(FILE *stream) {
            char key[9999],value[9999];
            bool ok = false;
            while(fscanf(stream,"%[^=]=%[^\n]\n",key,value)==2) {
                if(!strcmp(key,"END_OF_PARAMETERS")) {
                    ok = true;
                    break;
                }
                params(key) = value;
            }
            if(!ok) throw("paramters not properly terminated in save file");
        }
        // Print the parameters in some human-readable format.
        void pprint(FILE *stream=stdout,int depth=0) {
            narray<const char *> keys;
            params.keys(keys);
            for(int i=0;i<keys.length();i++) {
                fprintf(stream,"%*s",depth,"");
                fprintf(stream,"%s=%s\n",keys(i),params(keys(i)).ptr());
            }
        }

        virtual ~IComponent() {}

        // The following methods are obsolete for setting and getting parameters.
        // However, they cannot be converted automatically (since they might
        // trigger actions).

        virtual const char *command(const char **argv) {
            return 0;
        }

        virtual const char *command(const char *cmd,
                const char *arg1=0,
                const char *arg2=0,
                const char *arg3=0) {
            const char *argv[] = { cmd,arg1,arg2,arg3,0 };
            return command(argv);
        }

        /// Set a string property or throw an exception if not implemented.
        virtual void set(const char *key,const char *value) WARN_DEPRECATED {
            pset(key,value);
        }
        /// Set a number property or throw an exception if not implemented.
        virtual void set(const char *key,double value) WARN_DEPRECATED {
            pset(key,value);
        }
        /// Get a string property or throw an exception if not implemented.
        virtual const char *gets(const char *key) WARN_DEPRECATED {
            return pget(key);
        }

        /// Get a number property or throw an exception if not implemented.
        virtual double getd(const char *key) WARN_DEPRECATED {
            return pgetf(key);
        }
    };

    /// Loading and saving components.

    void save_component(FILE *stream,IComponent *classifier);
    IComponent *load_component(FILE *stream);

    /// Component registry.

    typedef IComponent *(*component_constructor)();
    void component_register_fun(const char *name,component_constructor f,bool replace=false);

    template <class T>
    inline IComponent *iconstructor() {
        return new T();
    }
    template <class T>
    inline void component_register(const char *name,bool replace=false) {
        component_constructor f = &iconstructor<T>;
        component_register_fun(name,f,replace);
    }
    template <class T,class S>
    inline IComponent *iconstructor2() {
        return new T(new S());
    }
    template <class T,class S>
    inline void component_register2(const char *name,bool replace=false) {
        component_constructor f = &iconstructor2<T,S>;
        component_register_fun(name,f,replace);
    }
    void list_components(narray<const char *> &names);
    component_constructor component_lookup(const char *name);
    IComponent *component_construct(const char *name);
}

#endif
