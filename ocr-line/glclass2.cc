// classifier implementations for glinerec

#include <unistd.h>
#include <sys/stat.h>
#include "glinerec.h"
#include "narray-binio.h"
#ifdef HAVE_GSL
#include "gsl.h"
#endif

extern "C" {
#include <sqlite3.h>
}

namespace glinerec {
    using namespace narray_io;

    struct SqliteDataset : IDataset {
        int n;
        int nc;
        int nf;
        SqliteDataset() {
            pdef("src","?","data source for current data");
            n = 0;
            nc = 0;
            nf = -1;
        }
        const char *name() {
            return "sqlitedataset";
        }
        void save(FILE *stream) {
            throw "meaningless";
        }
        void load(FILE *stream) {
            throw "meaningless";
        }
        sqlite3 *db;
        void open(const char *file) {
            debugf("info","binding to database %s\n",file);
            if(sqlite3_open_v2(file,&db,SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE,0)!=SQLITE_OK)
                throwf("%s: cannot open",file);
            // try to create the table (silently fails if it already exists)
            sqlite3_exec(db,"create table chars (src text,cls int,v blob)",0,0,0);
            sqlite3_exec(db,"PRAGMA synchronous=off",0,0,0);
        }
        void close() {
            sqlite3_close(db);
            db = 0;
        }
        void add(floatarray &v,int c) {
            narray<float8> v8;
            v8 = v;
            const char *cmd = "insert into chars (src,cls,v) values (?,?,?)";
            sqlite3_stmt *stmt;
            CHECK(sqlite3_prepare(db,cmd,-1,&stmt,0)==SQLITE_OK);
            const char *s = pget("src");
            CHECK(sqlite3_bind_text(stmt,1,s,strlen(s),SQLITE_TRANSIENT)==SQLITE_OK);
            CHECK(sqlite3_bind_int(stmt,2,c)==SQLITE_OK);
            CHECK(sqlite3_bind_blob(stmt,3,&v8(0),v8.length(),SQLITE_TRANSIENT)==SQLITE_OK);
            CHECK(sqlite3_step(stmt)==SQLITE_DONE);
            CHECK(sqlite3_finalize(stmt)==SQLITE_OK);
#if 0
            ochar *err = 0;
            if(sqlite3_exec(db,"COMMIT",0,0,&err)!=SQLITE_OK)
                throwf("%s:COMMIT",err);
#endif
            debugf("debug","inserted\n");
            n++;
            if(c>=nc) nc = c+1;
            if(nf<0) nf = v.length();
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
            pdef("data","data.sqlite","database file name");
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
        int nfeatures() {
            return nf;
        }
        int nclasses() {
            return nc;
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
                ds->open(pget("data"));
            }
            if(c>=nc) nc = c+1;
            if(nf<0) nf = v.length();
            else CHECK(nf==v.length());
            ds->add(v,c);
            n++;
        }
        void updateModel() {
        }
        // ignore these
        void save(FILE *stream) {
            debugf("warn","%s: ignoring save; data in %s",name(),pget("data"));
        }
        void load(FILE *stream) {
            debugf("warn","%s: ignoring load; data in %s",name(),pget("data"));
        }
        // we can't classify
        int classify(floatarray &x) {
            throw Unimplemented();
        }
        float outputs(floatarray &z,floatarray &x) {
            throw Unimplemented();
        }
    };

    void init_glclass2() {
        static bool init = false;
        if(init) return;
        init = true;

        component_register<SqliteDataset>("sqliteds");
        component_register<SqliteBuffer>("sqlitebuffer");
    }
}
