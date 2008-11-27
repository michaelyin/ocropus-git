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
# Purpose: building OCRopus
# Responsible: kofler
# Reviewer: 
# Primary Repository: http://ocropus.googlecode.com/svn/trunk/
# Web Sites: www.iupr.org, www.dfki.de

# first build this (libocropus) and then ocroscript
SUBDIRS = . ocroscript

# the folder where all ocropus headers will be installed
ocropusincludedir=$(includedir)/ocropus

# include flags to all default subfolders, optional folders see below
# FIXME clean up ocr-* subdirectories and remove them from this list;
# public header files should only go into include/
AM_CPPFLAGS = -I$(srcdir)/include \
                -I$(srcdir)/ext/voronoi \
                -I$(srcdir)/ocr-utils \
		-I@iulibheaders@ \
		-I@colibheaders@ \
		-I@colibheaders@/..

# the lib to compile
lib_LIBRARIES = libocropus.a

# the default files to compile into libocropus
libocropus_a_SOURCES  =  \
$(srcdir)/ext/voronoi/bit_func.cc \
$(srcdir)/ext/voronoi/cline.cc \
$(srcdir)/ext/voronoi/dinfo.cc \
$(srcdir)/ext/voronoi/draw_line.cc \
$(srcdir)/ext/voronoi/edgelist.cc \
$(srcdir)/ext/voronoi/erase.cc \
$(srcdir)/ext/voronoi/geometry.cc \
$(srcdir)/ext/voronoi/hash.cc \
$(srcdir)/ext/voronoi/heap.cc \
$(srcdir)/ext/voronoi/img_to_site.cc \
$(srcdir)/ext/voronoi/label_func.cc \
$(srcdir)/ext/voronoi/memory.cc \
$(srcdir)/ext/voronoi/output.cc \
$(srcdir)/ext/voronoi/read_image.cc \
$(srcdir)/ext/voronoi/sites.cc \
$(srcdir)/ext/voronoi/usage.cc \
$(srcdir)/ext/voronoi/voronoi-ocropus.cc \
$(srcdir)/ext/voronoi/voronoi-pageseg.cc \
$(srcdir)/ext/voronoi/voronoi.cc \
$(srcdir)/ocr-binarize/ocr-binarize-otsu.cc \
$(srcdir)/ocr-binarize/ocr-binarize-range.cc \
$(srcdir)/ocr-binarize/ocr-binarize-sauvola.cc \
$(srcdir)/ocr-pfst/a-star.cc \
$(srcdir)/ocr-pfst/beam-search.cc \
$(srcdir)/ocr-pfst/fst-em.cc \
$(srcdir)/ocr-pfst/fst-io.cc \
$(srcdir)/ocr-pfst/langmod-shortest-path.cc \
$(srcdir)/ocr-pfst/lattice.cc \
$(srcdir)/ocr-layout/line-info.cc \
$(srcdir)/ocr-layout/lines.cc \
$(srcdir)/ocr-layout/log-reg-data.cc \
$(srcdir)/ocr-layout/ocr-char-stats.cc \
$(srcdir)/ocr-layout/ocr-classify-zones.cc \
$(srcdir)/ocr-layout/ocr-color-encode-layout.cc \
$(srcdir)/ocr-layout/ocr-ctextline-rast-extended.cc \
$(srcdir)/ocr-layout/ocr-ctextline-rast.cc \
$(srcdir)/ocr-layout/ocr-deskew-rast.cc \
$(srcdir)/ocr-layout/ocr-detect-columns.cc \
$(srcdir)/ocr-layout/ocr-detect-paragraphs.cc \
$(srcdir)/ocr-layout/ocr-doc-clean-concomp.cc \
$(srcdir)/ocr-layout/ocr-doc-clean.cc \
$(srcdir)/ocr-layout/ocr-extract-gutters.cc \
$(srcdir)/ocr-layout/ocr-extract-rulings.cc \
$(srcdir)/ocr-layout/ocr-layout-1cp.cc \
$(srcdir)/ocr-layout/ocr-layout-rast.cc \
$(srcdir)/ocr-layout/ocr-layout-smear.cc \
$(srcdir)/ocr-layout/ocr-noisefilter.cc \
$(srcdir)/ocr-layout/ocr-pageframe-rast.cc \
$(srcdir)/ocr-layout/ocr-pageseg-wcuts.cc \
$(srcdir)/ocr-layout/ocr-pageseg-xycut.cc \
$(srcdir)/ocr-layout/ocr-reading-order.cc \
$(srcdir)/ocr-layout/ocr-segmentations.cc \
$(srcdir)/ocr-layout/ocr-text-image-seg.cc \
$(srcdir)/ocr-layout/ocr-visualize-layout-rast.cc \
$(srcdir)/ocr-layout/ocr-whitespace-cover.cc \
$(srcdir)/ocr-layout/ocr-word-segmentation.cc \
$(srcdir)/ocr-layout/recognized-page.cc \
$(srcdir)/ocr-leptonica/ocr-text-image-seg-leptonica.cc \
$(srcdir)/ocr-lineseg/ocr-cseg-ccs.cc \
$(srcdir)/ocr-lineseg/ocr-cseg-projection.cc \
$(srcdir)/ocr-lineseg/seg-ccs.cc \
$(srcdir)/ocr-lineseg/seg-cuts.cc \
$(srcdir)/ocr-lineseg/seg-eval.cc \
$(srcdir)/ocr-lineseg/seg-skel.cc \
$(srcdir)/ocr-utils/didegrade.cc \
$(srcdir)/ocr-utils/editdist.cc \
$(srcdir)/ocr-utils/grouper.cc \
$(srcdir)/ocr-utils/linesegs.cc \
$(srcdir)/ocr-utils/logger.cc \
$(srcdir)/ocr-utils/narray-io.cc \
$(srcdir)/ocr-utils/ocr-utils.cc \
$(srcdir)/ocr-utils/pagesegs.cc \
$(srcdir)/ocr-utils/resource-path.cc \
$(srcdir)/ocr-utils/sysutil.cc \
$(srcdir)/ocr-utils/xml-entities.cc

# the default headers to install
ocropusinclude_HEADERS =  \
$(srcdir)/include/ocropus.h \
$(srcdir)/include/ocr-pfst.h \
$(srcdir)/include/ocr-layout.h

# folders for installing models and words
modeldir=${datadir}/ocropus/models
worddir=${datadir}/ocropus/words

# install the data
#model_DATA = $(srcdir)/data/models/*
#word_DATA = $(srcdir)/data/words/*

# handling optional open-fst part
if ! nofst
    AM_CPPFLAGS += -DHAVE_FST
    libocropus_a_SOURCES += \
        $(srcdir)/ocr-openfst/fstbuilder.cc \
        $(srcdir)/ocr-openfst/fstutil.cc \
        $(srcdir)/ocr-openfst/fstmodels.cc
    ocropusinclude_HEADERS += $(srcdir)/include/ocr-openfst.h
endif

# handling optional tesseract part
if ! notesseract
    AM_CPPFLAGS += -I$(srcdir)/ocr-tesseract -I@tessheaders@ -DHAVE_TESSERACT
    libocropus_a_SOURCES += $(srcdir)/ocr-layout/tesseract.cc $(srcdir)/ocr-layout/recognized-page.cc
endif

# handling optional leptonica part
if use_leptonica
    AM_CPPFLAGS += -I@leptheaders@ -DHAVE_LEPTONICA
    ocropusinclude_HEADERS += $(srcdir)/ocr-leptonica/ocr-text-image-seg-leptonica.h
endif



# TODO compile all main-*.cc files in place
# !!! there must be a simpler way which also puts the file in the subfolder !!!
# noinst_PROGRAMS = main-feature-extractor
# main_feature_extractor_SOURCES = $(srcdir)/ocr-bpnet/main-feature-extractor-main.cc
# main_feature_extractor_LDADD = libocropus.a


# template for AM_CONDITIONALs
# if ! no
#     libocropus_a_SOURCES += $(srcdir)/
#     ocropusinclude_HEADERS += $(srcdir)/
# endif

# run check-style everytime and give a hint about make check
all:
	$(srcdir)/utilities/check-style -f $(srcdir)
	@echo
	@echo "Use 'make check' to run tests!"
	@echo
