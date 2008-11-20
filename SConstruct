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

# IMPORTANT
# 
# The official build system for OCRopus is autoconf.
# However, OCRopus should build with this build file on Ubuntu.
# If OCRopus doesn't build with this build file on Ubuntu,
# chances are that some file or file name violates the coding
# conventions or that there are new, unexpected external library
# dependencies.
#
# Please do not make significant modifications to this file
# without talking to tmb first.

print
print "**********************************************"
print "*** SConstruct is unsupported.             ***"
print "*** The official build system is autoconf. ***"
print "**********************************************"
print

EnsureSConsVersion(0,97)
from SCons.Script import *
import os,sys,string,re
from glob import glob

################################################################
### ocropus source files
################################################################

os.system("./generate_version_cc.sh ./version.cc")

sources = glob("ocr-*/*.cc") + glob("ext/voronoi/*.cc") + ["version.cc"]
exclude = r'.*/(main|test|bigtest)-.*\.cc'
sources = [f for f in sources if not re.search(exclude,f)]
packages = glob("ocroscript/*.pkg")
# headers = glob("ocr-*/*.h") + glob("ext/voronoi/*.h") + glob("include/*.h")
headers = glob("include/*.h")

################################################################
### command line options
################################################################

opts = Options('custom.py')
opts.Add('opt', 'Compiler flags for optimization/debugging', "-g -O")
opts.Add('warn', 'Compiler flags for warnings',
         "-Wall -Wno-sign-compare -Wno-write-strings -D__warn_unused_result__=__far__"+
         " -D_BACKWARD_BACKWARD_WARNING_H=1")
### path options
opts.Add(PathOption('prefix', 'The installation root for OCRopus ', "/usr/local"))
opts.Add(PathOption('iulib', 'The installation root of iulib', "/usr/local"))
opts.Add(PathOption('tesseract', 'The installation root of tesseract', "/usr/local"))
opts.Add(PathOption('leptonica', 'The installation root of leptonica', "/usr/local"))

### configuration options
opts.Add(BoolOption('openfst', 'Set to build with openFST', "yes"))

### optional build steps
opts.Add(BoolOption('test', "Run some tests after the build", "yes"))
opts.Add(BoolOption('style', 'Check style', "yes"))

env = Environment(options=opts, CXXFLAGS="${opt} ${warn}")
Help(opts.GenerateHelpText(env))
conf = Configure(env)

if "-DUNSAFE" in env["opt"]:
    print "WARNING: do not compile with -DUNSAFE except for benchmarking or profiling"
if re.search(r'-O[234]',env["opt"]):
    print "NOTE: compile with high optimization only for production use"
else:
    print "NOTE: compiling for development (slower but safer)"

################################################################
### lua/tolua++
################################################################

TOLUADIR = "ext/tolua++"
TOLUALIB = TOLUADIR+"/libtolua++.a"
TOLUA = TOLUADIR+"/tolua++"
LUADIR = "ext/lua"
LUALIB = LUADIR+"/liblua.a"

if not os.path.exists(LUALIB):
    print "\n###\n### building liblua.a recursively\n###\n"
    os.system("cd "+LUADIR+" && scons")
if not os.path.exists(TOLUA) or not os.path.exists(TOLUALIB):
    print "\n###\n### building tolua++ recursively\n###\n"
    os.system("cd "+TOLUADIR+" && scons")
tolua_builder = \
    Builder(action = TOLUA+" -L ocroscript/hook_exceptions.lua -o $TARGET $SOURCE",
            suffix=".cc",src_suffix=".pkg")
env.Append(BUILDERS = {'Tolua' : tolua_builder })
env.Append(LIBS=[File(TOLUALIB),File(LUALIB)])
env.Append(CPPPATH=["ext/tolua++","ext/lua"])

################################################################
### libraries
################################################################

### iulib

env.Append(LIBPATH=["${iulib}/lib"])
env.Append(CPPPATH=["${iulib}/include","${iulib}/include/colib","${iulib}/include/iulib"])
env.Append(LIBS=["iulib"])
assert conf.CheckHeader("colib.h",language="C++")
assert conf.CheckLibWithHeader("iulib","imgio.h","C++");
assert conf.CheckLibWithHeader("iulib","imgmisc.h","C++");

### tesseract

env.Append(CPPPATH=["${tesseract}/include/tesseract"])
env.Append(LIBPATH=["${tesseract}/lib"])
env.Append(LIBS=["libtesseract_full.a","pthread"])
env.Append(CPPDEFINES=["HAVE_TESSERACT"])
assert conf.CheckLibWithHeader('tesseract_full', 'tesseract/baseapi.h', 'C++')
assert conf.CheckLib('pthread')

### editline

env.Append(CPPDEFINES=["LUA_USE_EDITLINE"])
env.Append(LIBS=["editline"])
assert conf.CheckLib('editline', 'readline')

### OpenFST

if env["openfst"]:
    env.Append(LIBS=["fst"])
    env.Append(CPPDEFINES=['HAVE_FST'])
    assert conf.CheckLibWithHeader('fst', 'fst/lib/fst.h', 'C++')
else:
    sources = [s for s in sources if not "/fst" in s]

### SDL

env.Append(LIBS=["SDL","SDL_gfx"])
assert conf.CheckLibWithHeader('SDL', 'SDL/SDL.h', 'C')
assert conf.CheckLibWithHeader('SDL_gfx', 'SDL/SDL_gfxPrimitives.h', 'C')

### TIFF, JPEG, PNG

env.Append(LIBS=["tiff","jpeg","png"])
assert conf.CheckLib('tiff')
assert conf.CheckLib('jpeg')
assert conf.CheckLib('png')

### Leptonica

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

conf.Finish()

################################################################
### main targets
################################################################

env.Append(CPPPATH=["ext/voronoi"])
env.Append(CPPPATH=["include",
                    "ocr-binarize",
                    "ocr-deskew-rast",
                    "ocr-doc-clean",
                    "ocr-langmods",
                    "ocr-layout-rast",
                    "ocr-leptonica",
                    "ocr-lineseg",
                    "ocr-pageseg",
                    "ocr-samples",
                    "ocr-tesseract",
                    "ocr-utils"])

#env.Append(CPPPATH=glob("ocr-*"))
env.Append(LIBPATH=['.'])

libocropus = env.StaticLibrary('libocropus.a',sources)
for p in packages: env.Tolua(p)

env.Prepend(LIBS=[File("libocropus.a")])
ocroscript = env.Program('ocroscript/ocroscript',
                         ["ocroscript/ocroscript.cc", "ocroscript/ocrotoplevel.cc"] + 
                         [re.sub(r'\.pkg','.cc',p) for p in packages])

################################################################
### install
################################################################

prefix = "${prefix}"
incdir = prefix+"/include/ocropus"
libdir = prefix+"/lib"
datadir = prefix+"/share/ocropus"
bindir = prefix+"/bin"
scriptsdir = datadir + '/scripts'

env.Install(libdir,libocropus)
env.Install(bindir,ocroscript)
env.Install(scriptsdir, glob('ocroscript/scripts/*.lua'))
env.Install(scriptsdir + '/lib', glob('ocroscript/scripts/lib/*.lua'))
env.Install(datadir + '/models', glob('data/models/*'))
env.Install(datadir + '/words', glob('data/words/*'))
for header in headers: env.Install(incdir,header)

env.Alias('install',bindir)
env.Alias('install',libdir)
env.Alias('install',incdir)
env.Alias('install',datadir)

################################################################
### unit tests
################################################################

test_builder = Builder(action='$SOURCE && touch $TARGET',
                  suffix = '.passed',
                  src_suffix = '')
env.Append(BUILDERS={'Test':test_builder})
if env["test"]:
    test = env.Test("ocroscript/run-tests")
    env.Depends(test,'ocroscript/ocroscript')
#env.Depends(test,glob.glob('ocroscript/scripts/*'))
#env.Test("ocroscript/run-toplevels")

################################################################
### style checking
################################################################

if env["style"]:
    os.system("utilities/check-style -f ocr-*")
