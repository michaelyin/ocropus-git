# -*- python -*-
# vi: ft=python

# Copyright 2008 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz 
# or its licensors, as applicable.
# 
# You may not use this file except under the terms of the accompanying license.
# 
# Licensed under the Apache License, Version 2.0 (the "License"); you
# may not use this file except in compliance with the License. You may
# obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# 
# Project: OCRopus - the open source document analysis and OCR system
# File: SConstruct
# Purpose: building OCRopus
# Responsible: tmb
# Reviewer: 
# Primary Repository: http://ocropus.googlecode.com/svn/trunk/
# Web Sites: www.iupr.org, www.dfki.de

import os
if os.system("uname -a | egrep 'Ubuntu.*2009' > /dev/null")!=0:
    print "WARNING: scons not supported on platforms other than Ubuntu 9.10"

EnsureSConsVersion(0,97)
from SCons.Script import *
import os,sys,string,re
from glob import glob

################################################################
### ocropus source files
################################################################

os.system("./generate_version_cc.sh ./version.cc")

sources = glob("ocr-*/*.cc") + ["version.cc"]
exclude = r'.*/(main|test|bigtest)-.*\.cc'
sources = [f for f in sources if not re.search(exclude,f)]
headers = glob("*/*.h")

################################################################
### command line options
################################################################

opts = Variables('custom.py')
opts.Add('opt', 'Compiler flags for optimization/debugging', "-O2")
opts.Add('warn', 'Compiler flags for warnings',
         "-Wall -Wno-sign-compare -Wno-write-strings -Wno-unknown-pragmas "+
         " -D__warn_unused_result__=__far__"+
         " -D_BACKWARD_BACKWARD_WARNING_H=1")
### path options
opts.Add(PathVariable('prefix', 'The installation root for OCRopus ', "/usr/local"))
opts.Add(PathVariable('iulib', 'The installation root of iulib', "/usr/local"))
opts.Add(PathVariable('destdir', 'Destination root directory', "", PathVariable.PathAccept))
opts.Add(PathVariable('leptonica', 'The installation root of leptonica', "/usr/local"))

opts.Add(BoolVariable('gsl', "use GSL-dependent features", "no"))
opts.Add(BoolVariable('omp', "use OpenMP", "no"))
opts.Add(BoolVariable('lept', "use Leptonica", "no"))
opts.Add(BoolVariable('sqlite3', "use sqlite3", "yes"))

opts.Add(BoolVariable('test', "Run some tests after the build", "no"))
opts.Add(BoolVariable('style', 'Check style', "no"))


destdir = "${destdir}"
prefix = "${prefix}"
incdir = prefix+"/include/ocropus"
libdir = prefix+"/lib"
datadir = prefix+"/share/ocropus"
bindir = prefix+"/bin"
scriptsdir = datadir + '/scripts'

env = Environment(options=opts)
env.Append(CXXFLAGS=["-DDATADIR='\""+datadir+"\"'"])
env.Append(CXXFLAGS=["-g","-fPIC"])
env.Append(CXXFLAGS=env["opt"])
env.Append(CXXFLAGS=env["warn"])
conf = Configure(env)
Help(opts.GenerateHelpText(env))

if 0:
    if "-DUNSAFE" in env["opt"]:
        print "WARNING: do not compile with -DUNSAFE except for benchmarking or profiling"
    if re.search(r'-O[234]',env["opt"]):
        print "NOTE: compile with high optimization only for production use"
    else:
        print "NOTE: compiling for development (slower but safer)"

################################################################
### libraries
################################################################

### iulib

env.Append(LIBPATH=["${iulib}/lib"])
env.Append(CPPPATH=["${iulib}/include"])
env.Append(LIBS=["iulib"])
assert conf.CheckLibWithHeader("iulib","iulib/iulib.h","C++");
assert conf.CheckHeader("colib/colib.h",language="C++")

# dl (do we need this?)

env.Append(LIBS=["dl"])
assert conf.CheckLib('dl')

### TIFF, JPEG, PNG

env.Append(LIBS=["tiff","jpeg","png","gif"])
assert conf.CheckLib('gif')
assert conf.CheckLib('tiff')
assert conf.CheckLib('jpeg')
assert conf.CheckLib('png')

# sources = [s for s in sources if not "/fst" in s]

### SDL (include if it's there in case iulib needs it)

if conf.CheckLibWithHeader('SDL', 'SDL/SDL.h', 'C'):
    if conf.CheckLibWithHeader('SDL_gfx', 'SDL/SDL_gfxPrimitives.h', 'C'):
        env.Append(LIBS=["SDL","SDL_gfx"])

### Leptonica

if env["lept"]:
    env.Append(CPPPATH='${leptonica}/include')
    env.Append(LIBS=["lept"])
    env.Append(CPPDEFINES=['HAVE_LEPTONICA'])
    if conf.CheckLibWithHeader('lept', ['stdlib.h', 'stdio.h', 'leptonica/allheaders.h'], 'C'):
        # This happens if you install it with apt-get install libleptonica-dev.
        env.Append(CPPPATH=['/usr/include/leptonica',
                            '/usr/local/include/leptonica',
                            '${leptonica}/include/leptonica'])
    elif conf.CheckLibWithHeader('lept', ['stdlib.h', 'stdio.h', 'liblept/allheaders.h'], 'C'):
        # This happens if you install from a tarball.
        env.Append(CPPPATH=['/usr/include/liblept',
                            '/usr/local/include/liblept',
                            '${leptonica}/include/liblept'])
    else:
        # And this probably doesn't happen unless you manually specify the path.
        assert conf.CheckLibWithHeader('lept', ['stdlib.h', 'stdio.h', 'allheaders.h'], 'C')
else:
    sources = [s for s in sources if not "leptonica" in s]

if env["sqlite3"]:
    env.Append(CPPDEFINES=['HAVE_SQLITE3'])
    env.Append(LIBS=["sqlite3"])

### gsl

if env["gsl"]:
    env.Append(CPPDEFINES=['HAVE_GSL'])
    env.Append(LIBS=["gsl","blas"])

# enable OpenMP for high optimization

if env["omp"]:
    env.Append(CXXFLAGS=["-fopenmp"])
    env.Append(LINKFLAGS=["-fopenmp"])

conf.Finish()

################################################################
### main targets
################################################################

env.Append(CPPPATH=glob("ocr-*"))

#env.Append(CPPPATH=glob("ocr-*"))
env.Append(LIBPATH=['.'])

# libocropus = env.StaticLibrary('libocropus.a',sources)
libocropus = env.SharedLibrary('libocropus',sources)
# env.Prepend(LIBS=[File("libocropus.so")])

################################################################
### install
################################################################

env.Install(destdir+libdir,libocropus)
env.Install(destdir+datadir + '/models', glob('data/models/*'))
env.Install(destdir+datadir + '/words', glob('data/words/*'))
for header in headers: env.Install(destdir+incdir,header)
for file in glob('data/models/*.gz'):
    base = re.sub(r'\.gz$','',file)
    base = re.sub(r'^[./]*data/','',base)
    base = destdir+datadir+"/"+base
    env.Command(base,file,"gunzip -9v < %s > %s" % (file,base))
    env.Alias('install',base)

env.Alias('install',destdir+bindir)
env.Alias('install',destdir+libdir)
env.Alias('install',destdir+incdir)
env.Alias('install',destdir+datadir)

################################################################
### commands
################################################################

penv = env.Clone()
penv.Append(LIBS=[File("libocropus.so")])

for cmd in glob("commands/*.cc"): 
    penv.Program(cmd,LIBS=File("libocropus.so"))
    penv.Install(destdir+bindir,re.sub('.cc$','',cmd))

################################################################
### unit tests
################################################################


if env["test"]:
    test_builder = Builder(action='$SOURCE && touch $TARGET',
        suffix = '.passed',
        src_suffix = '')
    env.Append(BUILDERS={'Test':test_builder})
    for cmd in Glob("*/test-*.cc")+Glob("*/test*/test-*.cc"):
        cmd = str(cmd)
        penv.Program(cmd)
        print cmd
        cmd = re.sub('.cc$','',cmd)
        penv.Test(cmd)

################################################################
### style checking
################################################################

if env["style"]:
    os.system("utilities/check-style -f ocr-*")
