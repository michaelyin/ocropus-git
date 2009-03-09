#!/usr/bin/python

import glob, os, sys, string as s

# cc files from ocr-* folders are compiled into libocropus by default

# extra directories for libocropus
extradirs = """
    ext/voronoi
""".split()

# only add these files if tesseract is enabled
tess = """
    ocr-tesseract/tesseract.cc
    ocr-layout/recognized-page.cc
""".split()

# extra files which should not go to libocropus
exclude = """

""".split()

# optional files
exclude += tess

# some stupid flags for switching between writing to Makefile.am and stdout
TLAM = True
OSAM = False
# disable writing of Top Level AM file if set to zero
if len(sys.argv) > 1 and int(sys.argv[1]) is 0:
    TLAM = False
# enable writing of OcroScript AM file if set to one
if len(sys.argv) > 2 and  int(sys.argv[2]) is 1:
    OSAM = True

def print_header():
    print """# Copyright 2008 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz
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
# File: Makefile.am
# Purpose: building OCRopus
# Responsible: kofler
# Reviewer:
# Primary Repository: http://ocropus.googlecode.com/svn/trunk/
# Web Sites: www.iupr.org, www.dfki.de
"""

if TLAM:
    # save stdout to put it back later
    saveout = sys.stdout
    # redirect output
    fsock = open('Makefile.am', 'w')
    sys.stdout = fsock

print_header()
print """
# first build this (libocropus) and then ocroscript
SUBDIRS = . ocroscript

# the folder where all ocropus headers will be installed
ocropusincludedir=$(includedir)/ocropus

AM_CPPFLAGS = -I$(srcdir)/include -I$(srcdir)/ocr-utils \
-I@iulibheaders@ -I@colibheaders@ -I@tessheaders@

AM_CXXFLAGS = $(CXXFLAGS) -Wall -Wno-sign-compare -Wno-write-strings -Wno-deprecated

lib_LIBRARIES = libocropus.a
"""

### libocroups

print "# the default files to compile into libocropus"
print "libocropus_a_SOURCES = ",
# gather files from ocr- folders
for cc in glob.glob("ocr-*/*.cc"):
    if cc in exclude: continue
    if os.path.basename(cc).startswith("main-"): continue
    if os.path.basename(cc).startswith("test-"): continue
    print "$(srcdir)/" + cc,
# gather files from extra folders, e.g. ext/voronoi
for d in extradirs:
    for cc in glob.glob(d + "/*.cc"):
        if cc in exclude: continue
        if os.path.basename(cc).startswith("main-"): continue
        if os.path.basename(cc).startswith("test-"): continue
        print "$(srcdir)/" + cc,

print """

# folders for installing models and words
#modeldir=${datadir}/ocropus/models
worddir=${datadir}/ocropus/words

# install the data
#model_DATA = $(srcdir)/data/models/*
word_DATA = $(srcdir)/data/words/*
"""

# optional stuff
print
print "if ! notesseract"
print "    AM_CPPFLAGS += -I@tessheaders@ -DHAVE_TESSERACT"
print "    libocropus_a_SOURCES +=" + s.join(" $(srcdir)/"+f for f in tess)
print "endif"
print
print "if use_leptonica"
print "    AM_CPPFLAGS += -I@leptheaders@ -DHAVE_LEPTONICA"
print "endif"

print
print
print "ocropusinclude_HEADERS = ",
for h in glob.glob("include/*.h"):
    print "$(srcdir)/" + h,
for h in glob.glob("ocr-utils/*.h"):
    print "$(srcdir)/" + h,
print
print

# gather all main-* files
mains = glob.glob("*/main-*.cc")
# name the resulting binaries (strip folder and suffix)
print "noinst_PROGRAMS = " + s.join(" " + os.path.basename(m)[:-3] for m in mains)
for m in mains:
    mName = os.path.basename(m)[:-3].replace('-','_')
    print mName + "_SOURCES = $(srcdir)/" + m
    print mName + "_LDADD = libocropus.a"
print

# gather all test-* files
tests = glob.glob("*/test-*.cc")
tests += glob.glob("*/tests/test-*.cc")
# name the resulting binaries (strip folder and suffix)
print "check_PROGRAMS = " + s.join(" " + os.path.basename(t)[:-3] for t in tests)
for t in tests:
    tName = os.path.basename(t)[:-3].replace('-','_')
    print tName + "_SOURCES = $(srcdir)/" + t
    print tName + "_LDADD = libocropus.a"
    print tName + "_CPPFLAGS = -I$(srcdir)/include -I$(srcdir)/ocr-utils \\"
    print "-I@iulibheaders@ -I@colibheaders@",
    print "-I@tessheaders@"

# run all test-* binaries with make check
print
print "check:"
print '	@echo "# running tests"'
for t in tests:
    print "	$(srcdir)/" + os.path.basename(t)[:-3] + " $(srcdir)/ocroscript/tests/images"


print """
# run check-style everytime and give a hint about make check
all:
	$(srcdir)/utilities/check-style -f $(srcdir)
	@echo
	@echo "Use 'make check' to run tests!"
	@echo
"""

if TLAM:
    # reset stdout and close file
    sys.stdout = saveout
    fsock.close()



# FIXME look at ocroscript Makefile.am again! stop here for now!
sys.exit()

### libocroscript ##############################################################
# all paths need to be relative to ocroscript now !!!
###

if OSAM:
    # save stdou to put it back later
    saveout = sys.stdout
    # redirect output
    fsock = open('ocroscript/Makefile.am', 'w')
    sys.stdout = fsock


optpkgs = """
    ocroscript/lepton.pkg
    ocroscript/sdl.pkg
""".split()

packages = [ p for p in glob.glob("ocroscript/*.pkg") 
             if not p in exclude and not p in optpkgs ] 
# add pkgs in other folders than ocroscript
for dir in dirs:
    packages += [ p for p in glob.glob(dir+"/*.pkg") if not p in exclude
                and not p in packages and not p in optpkgs ]


print 'TOLUADIR = $(srcdir)/../ext/tolua++'
print 'TOLUALIB = $(TOLUADIR)/libtolua++.a'
print 'TOLUA = $(TOLUADIR)/tolua++'
print 'LUADIR = $(srcdir)/../ext/lua'
print 'LUALIB = $(LUADIR)/liblua.a'

print "AM_CPPFLAGS = ",
for d in dirs:
    print "-I$(srcdir)/../"+d,
print "-I@iulibheaders@",
print "-I@tessheaders@",
print "-I$(TOLUADIR)"
print

print
print "lib_LIBRARIES = libocroscript.a"

print "libocroscript_a_SOURCES =",
print s.join(" $(srcdir)/../"+f.replace(".pkg", ".cc") for f in packages)

#print "libocroscript.a: $(TOLUA)"

print
# make pkg depend on cc
for p in packages:
    print "$(srcdir)/../" + p.replace(".pkg", ".cc") + " : $(srcdir)/../" + p,
    print " $(TOLUA)"
    print "	$(TOLUA) -L $(srcdir)/../ocroscript/hook_exceptions.lua -o $@ $<"
# build cc from pkg
print ".pkg.cc: "
print "	$(TOLUA) -L $(srcdir)/../ocroscript/hook_exceptions.lua -o $@ $<"

print "$(TOLUA):"
print "	cd $(TOLUADIR) && $(MAKE)"



if OSAM:
    # reset stdout and close file
    sys.stdout = saveout
    fsock.close()


