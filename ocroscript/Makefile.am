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
# File: Makefile.am
# Purpose: building ocrocsript
# Responsible: kofler
# Reviewer: 
# Primary Repository: http://ocropus.googlecode.com/svn/trunk/
# Web Sites: www.iupr.org, www.dfki.de

TOLUADIR = $(srcdir)/../ext/tolua++
TOLUALIB = $(TOLUADIR)/libtolua++.a
TOLUA = $(TOLUADIR)/tolua++
LUADIR = $(srcdir)/../ext/lua
LUALIB = $(LUADIR)/liblua.a
AM_CPPFLAGS =  -I$(srcdir)/../include -I$(srcdir)/../ocr-utils \
	-I@iulibheaders@ -I@colibheaders@ -I@colibheaders@/.. \
	-I$(TOLUADIR) -I$(LUADIR)

# having /../ocroscript/ all over the place looks a bit ugly, but later we
# will have .pkg files also in other subfolders. The check-am.py script
# checks paths from top-level, that's the reason for the long versions.

lib_LIBRARIES = libocroscript.a
libocroscript_a_SOURCES =  $(srcdir)/graphics.cc  $(srcdir)/sysutil.cc  $(srcdir)/image.cc  $(srcdir)/imgrlecmds.cc $(srcdir)/imgbitscmds.cc  $(srcdir)/ocr.cc  $(srcdir)/narray.cc $(srcdir)/pfst.cc $(srcdir)/../version.cc

bin_PROGRAMS = ocroscript
ocroscript_SOURCES = $(srcdir)/../ocroscript/ocroscript.cc $(srcdir)/../ocroscript/ocrotoplevel.cc 

ocroscript_LDADD = -ltiff -lpng -ljpeg $(LUALIB) libocroscript.a $(srcdir)/../libocropus.a  $(TOLUALIB)

# TODO depend on libocropus.a, i.e. compile it if not there!

if ! nofst
    AM_CPPFLAGS += -DHAVE_FST
    libocroscript_a_SOURCES += $(srcdir)/openfst.cc
endif

# FIXME incorporate tesseract again somehow
#if ! notesseract
#    AM_CPPFLAGS += -I$(srcdir)/../ocr-tesseract -I@tessheaders@ -DHAVE_TESSERACT
#    libocroscript_a_SOURCES +=  $(srcdir)/tess.cc
#else
#    libocroscript_a_SOURCES +=  $(srcdir)/tess-stub.cc
#endif

if use_editline
    AM_CPPFLAGS += -DLUA_USE_EDITLINE
    ocroscript_LDADD += -leditline
endif

if use_leptonica
    AM_CPPFLAGS += -DHAVE_LEPTONICA
    ocroscript_LDADD += -llept
endif

# install scripts
scriptsdir=${datadir}/ocropus
nobase_scripts_DATA = $(srcdir)/scripts/*.lua
nobase_scripts_DATA += $(srcdir)/scripts/lib/*.lua


$(srcdir)/graphics.cc : $(srcdir)/graphics.pkg  $(TOLUA)
$(srcdir)/sysutil.cc : $(srcdir)/sysutil.pkg  $(TOLUA)
$(srcdir)/image.cc : $(srcdir)/image.pkg  $(TOLUA)
$(srcdir)/imgrlecmds.cc : $(srcdir)/imgrlecmds.pkg  $(TOLUA)
$(srcdir)/imgbitscmds.cc : $(srcdir)/imgbitscmds.pkg  $(TOLUA)
# $(srcdir)/tess.cc : $(srcdir)/tess.pkg  $(TOLUA)
$(srcdir)/ocr.cc : $(srcdir)/ocr.pkg  $(TOLUA)
$(srcdir)/pfst.cc : $(srcdir)/pfst.pkg  $(TOLUA)
$(srcdir)/narray.cc : $(srcdir)/narray.pkg  $(TOLUA)
$(srcdir)/fst.cc : $(srcdir)/fst.pkg  $(TOLUA)
$(srcdir)/leptonica.cc : $(srcdir)/leptonica.pkg  $(TOLUA)

$(srcdir)/../version.cc :
	$(srcdir)/../generate_version_cc.sh $(srcdir)/../version.cc

.pkg.cc:
	$(TOLUA) -L $(srcdir)/hook_exceptions.lua -o $@ $<
$(TOLUA):
	cd $(TOLUADIR) && $(MAKE)

check:
	$(srcdir)/run-tests && touch $(srcdir)/run-tests.passed
	$(srcdir)/run-toplevels


