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
		-I$(srcdir)/ocr-langmods \
		-I$(srcdir)/ocr-lineseg \
		-I$(srcdir)/ocr-binarize \
		-I$(srcdir)/ocr-deskew-rast \
		-I$(srcdir)/ocr-doc-clean \
		-I$(srcdir)/ocr-layout-rast \
		-I$(srcdir)/ocr-pageseg \
		-I$(srcdir)/ocr-samples \
		-I$(srcdir)/ocr-utils \
		-I$(srcdir)/ocr-leptonica \
		-I@iulibheaders@ \
		-I@colibheaders@ \
		-I@colibheaders@/..

# the lib to compile
lib_LIBRARIES = libocropus.a

# the default files to compile into libocropus
libocropus_a_SOURCES  =  \
$(srcdir)/ext/voronoi/voronoi.cc \
$(srcdir)/ext/voronoi/dinfo.cc \
$(srcdir)/ext/voronoi/erase.cc \
$(srcdir)/ext/voronoi/img_to_site.cc \
$(srcdir)/ext/voronoi/usage.cc \
$(srcdir)/ext/voronoi/draw_line.cc \
$(srcdir)/ext/voronoi/memory.cc \
$(srcdir)/ext/voronoi/read_image.cc \
$(srcdir)/ext/voronoi/heap.cc \
$(srcdir)/ext/voronoi/hash.cc \
$(srcdir)/ext/voronoi/bit_func.cc \
$(srcdir)/ext/voronoi/geometry.cc \
$(srcdir)/ext/voronoi/output.cc \
$(srcdir)/ext/voronoi/edgelist.cc \
$(srcdir)/ext/voronoi/voronoi-pageseg.cc \
$(srcdir)/ext/voronoi/voronoi-ocropus.cc \
$(srcdir)/ext/voronoi/sites.cc \
$(srcdir)/ext/voronoi/cline.cc \
$(srcdir)/ext/voronoi/label_func.cc \
$(srcdir)/ocr-langmods/a-star.cc \
$(srcdir)/ocr-langmods/beam-search.cc \
$(srcdir)/ocr-langmods/a-star.cc \
$(srcdir)/ocr-langmods/fst-io.cc \
$(srcdir)/ocr-langmods/langmod-shortest-path.cc \
$(srcdir)/ocr-langmods/lattice.cc \
$(srcdir)/ocr-langmods/fst-em.cc \
$(srcdir)/ocr-binarize/ocr-binarize-sauvola.cc \
$(srcdir)/ocr-binarize/ocr-binarize-otsu.cc \
$(srcdir)/ocr-deskew-rast/ocr-deskew-rast.cc \
$(srcdir)/ocr-doc-clean/ocr-noisefilter.cc \
$(srcdir)/ocr-doc-clean/ocr-doc-clean.cc \
$(srcdir)/ocr-doc-clean/ocr-doc-clean-concomp.cc \
$(srcdir)/ocr-doc-clean/ocr-pageframe-rast.cc \
$(srcdir)/ocr-layout-rast/ocr-layout-rast.cc \
$(srcdir)/ocr-layout-rast/line-info.cc \
$(srcdir)/ocr-layout-rast/ocr-extract-gutters.cc \
$(srcdir)/ocr-layout-rast/ocr-ctextline-rast.cc \
$(srcdir)/ocr-layout-rast/ocr-whitespace-cover.cc \
$(srcdir)/ocr-layout-rast/ocr-char-stats.cc \
$(srcdir)/ocr-layout-rast/ocr-ctextline-rast-extended.cc \
$(srcdir)/ocr-pageseg/ocr-pageseg-wcuts.cc \
$(srcdir)/ocr-pageseg/ocr-text-image-seg.cc \
$(srcdir)/ocr-pageseg/log-reg-data.cc \
$(srcdir)/ocr-pageseg/ocr-pageseg-xycut.cc \
$(srcdir)/ocr-pageseg/ocr-classify-zones.cc \
$(srcdir)/ocr-pageseg/ocr-word-segmentation.cc \
$(srcdir)/ocr-leptonica/ocr-text-image-seg-leptonica.cc \
$(srcdir)/ocr-samples/ocr-layout-smear.cc \
$(srcdir)/ocr-samples/ocr-cseg-ccs.cc \
$(srcdir)/ocr-samples/ocr-cseg-projection.cc \
$(srcdir)/ocr-samples/kmeans.cc \
$(srcdir)/ocr-samples/ocr-binarize-range.cc \
$(srcdir)/ocr-samples/ocr-layout-1cp.cc \
$(srcdir)/ocr-utils/narray-io.cc \
$(srcdir)/ocr-utils/xml-entities.cc \
$(srcdir)/ocr-utils/ocr-segmentations.cc \
$(srcdir)/ocr-utils/didegrade.cc \
$(srcdir)/ocr-lineseg/seg-skel.cc \
$(srcdir)/ocr-utils/lines.cc \
$(srcdir)/ocr-utils/logger.cc \
$(srcdir)/ocr-utils/grouper.cc \
$(srcdir)/ocr-utils/idmap.cc \
$(srcdir)/ocr-utils/eigens.cc \
$(srcdir)/ocr-utils/sysutil.cc \
$(srcdir)/ocr-utils/ocr-utils.cc \
$(srcdir)/ocr-utils/resource-path.cc \
$(srcdir)/ocr-lineseg/seg-ccs.cc \
$(srcdir)/ocr-lineseg/seg-cuts.cc \
$(srcdir)/ocr-lineseg/seg-eval.cc \
$(srcdir)/ocr-utils/editdist.cc

# the default headers to install
ocropusinclude_HEADERS =  \
$(srcdir)/include/ocropus.h \
$(srcdir)/include/ocr-binarize.h \
$(srcdir)/include/ocr-deskew-rast-public.h \
$(srcdir)/include/ocr-doc-clean.h \
$(srcdir)/include/ocr-langmods.h \
$(srcdir)/include/ocr-layout-rast-public.h \
$(srcdir)/include/ocr-leptonica.h \
$(srcdir)/include/ocr-lineseg.h \
$(srcdir)/include/ocr-pageseg.h \
$(srcdir)/include/ocr-samples.h \
$(srcdir)/include/ocr-tesseract.h \
$(srcdir)/include/ocr-utils-public.h \
$(srcdir)/ext/voronoi/extern.h \
$(srcdir)/ext/voronoi/read_image.h \
$(srcdir)/ext/voronoi/function.h \
$(srcdir)/ext/voronoi/defs.h \
$(srcdir)/ext/voronoi/voronoi-ocropus.h \
$(srcdir)/ext/voronoi/const.h \
$(srcdir)/ocr-langmods/beam-search.h \
$(srcdir)/ocr-langmods/a-star.h \
$(srcdir)/ocr-langmods/fst-io.h \
$(srcdir)/ocr-langmods/langmod-shortest-path.h \
$(srcdir)/ocr-langmods/lattice.h \
$(srcdir)/ocr-langmods/fst-em.h \
$(srcdir)/ocr-leptonica/ocr-text-image-seg-leptonica.h \
$(srcdir)/ocr-binarize/ocr-binarize-sauvola.h \
$(srcdir)/ocr-binarize/ocr-binarize-otsu.h \
$(srcdir)/ocr-deskew-rast/ocr-deskew-rast.h \
$(srcdir)/ocr-doc-clean/ocr-noisefilter.h \
$(srcdir)/ocr-doc-clean/ocr-doc-clean.h \
$(srcdir)/ocr-doc-clean/ocr-doc-clean-concomp.h \
$(srcdir)/ocr-doc-clean/ocr-pageframe-rast.h \
$(srcdir)/ocr-layout-rast/ocr-whitespace-cover.h \
$(srcdir)/ocr-layout-rast/ocr-char-stats.h \
$(srcdir)/ocr-layout-rast/ocr-layout-rast.h \
$(srcdir)/ocr-layout-rast/line-info.h \
$(srcdir)/ocr-layout-rast/ocr-extract-gutters.h \
$(srcdir)/ocr-layout-rast/ocr-ctextline-rast.h \
$(srcdir)/ocr-layout-rast/ocr-ctextline-rast-extended.h \
$(srcdir)/ocr-pageseg/ocr-classify-zones.h \
$(srcdir)/ocr-pageseg/ocr-pageseg-wcuts.h \
$(srcdir)/ocr-pageseg/ocr-text-image-seg.h \
$(srcdir)/ocr-pageseg/log-reg-data.h \
$(srcdir)/ocr-pageseg/ocr-pageseg-xycut.h \
$(srcdir)/ocr-pageseg/ocr-word-segmentation.h \
$(srcdir)/ocr-samples/ocrcomponents.h \
$(srcdir)/ocr-samples/kmeans.h \
$(srcdir)/ocr-utils/idmap.h \
$(srcdir)/ocr-utils/narray-io.h \
$(srcdir)/ocr-utils/ocr-segmentations.h \
$(srcdir)/ocr-utils/eigens.h \
$(srcdir)/ocr-utils/sysutil.h \
$(srcdir)/ocr-utils/logger.h \
$(srcdir)/ocr-utils/grouper.h \
$(srcdir)/ocr-utils/segmentation.h \
$(srcdir)/ocr-utils/pages.h \
$(srcdir)/ocr-utils/didegrade.h \
$(srcdir)/ocr-utils/lines.h \
$(srcdir)/ocr-utils/regionextractor.h \
$(srcdir)/ocr-utils/ocr-utils.h \
$(srcdir)/ocr-utils/resource-path.h \
$(srcdir)/ocr-utils/queue.h \
$(srcdir)/ocr-utils/grid.h \
$(srcdir)/ocr-lineseg/seg-eval.h \
$(srcdir)/ocr-utils/enumerator.h \
$(srcdir)/ocr-utils/editdist.h \
$(srcdir)/ocr-utils/xml-entities.h

# folders for installing models and words
modeldir=${datadir}/ocropus/models
worddir=${datadir}/ocropus/words

# install the data
model_DATA = $(srcdir)/data/models/*
word_DATA = $(srcdir)/data/words/*

# handling optional open-fst part
if ! nofst
    AM_CPPFLAGS += -DHAVE_FST
    libocropus_a_SOURCES += $(srcdir)/ocr-langmods/fstbuilder.cc  $(srcdir)/ocr-langmods/fstutil.cc  $(srcdir)/ocr-langmods/fstmodels.cc
    ocropusinclude_HEADERS += $(srcdir)/ocr-langmods/fstutil.h $(srcdir)/ocr-langmods/fstmodels.h $(srcdir)/ocr-langmods/fstbuilder.h
endif

# handling optional tesseract part
if ! notesseract
    AM_CPPFLAGS += -I$(srcdir)/ocr-tesseract -I@tessheaders@ -DHAVE_TESSERACT
    libocropus_a_SOURCES += $(srcdir)/ocr-tesseract/tesseract.cc $(srcdir)/ocr-tesseract/recognized-page.cc
    ocropusinclude_HEADERS += $(srcdir)/ocr-tesseract/tesseract.h $(srcdir)/ocr-tesseract/recognized-page.h
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
	

