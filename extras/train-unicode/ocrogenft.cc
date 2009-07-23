// -*- C++ -*-

// Copyright 2009 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz
// or its licensors, as applicable.
//
// You may not use this file except under the terms of the accompanying license.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you
// may not use this file except in compliance with the License. You may
// obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Project: ocrogenft -- rendering artificial training data using freetype
// File: ocrogenft.cc
// Purpose: main command line program
// Responsible: remat
// Reviewer:
// Primary Repository: ~remat/mercurial/ocrogenft
// Web Sites: www.iupr.org, www.dfki.de


#include <sys/stat.h>
#include <sys/types.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <wchar.h>
#include <iulib.h>
#include <colib.h>
#include <bookstore.h>
#include <didegrade.h>
#include <locale.h>
#include <getopt.h>
#include <errno.h>
#include <iustring.h>
#include <time.h>
#include <ctype.h>

static const double PI = 3.1415926535;

int cWidth = 32;
int cHeight = 0;
int hDpi = 96;
int vDpi = 0;
int nLines = 40;
narray<strg> inputFilenames;
autodel<ocropus::IBookStore> bookStore;

strg outDir = ".";
strg bookName;
bool writePages = false;
strg ttfFile = "/usr/share/fonts/truetype/freefont/FreeSans.ttf";
strg logFilename = "log.txt";
int verbose = 0;

// -- parameters for baird's degradation --
double jitter_mean = 0.1;
double jitter_sigma = 0.05;
double sensitivity_mean = .125;
double sensitivity_sigma = .04;
double threshold_mean = 0.4;
double threshold_sigma = .04;

double min_xRad = 0.0;
double max_xRad = 2*PI;
double min_yRad = 0.0;
double max_yRad = 2*PI;
double min_xAmpl = 2.0;
double max_xAmpl = 5.0;
double min_yAmpl = 2.0;
double max_yAmpl = 5.0;

// -- constant seed to ensure deterministic --
int seed = 123456789;

void printUsage(int argc, char *argv[]);
void parseArgs(int argc, char *argv[]);
void printArgs(FILE* file);

using namespace colib;
using namespace iulib;

int renderText(intarray& seg, bytearray& gray, ustrg& string, int x0, int y0, FT_Face& face) {
    bool onePixel = false;
    for(int i=0; i<string.length(); i++) {
        int error = FT_Load_Char(face, string[i].ord(), FT_LOAD_RENDER);
        if(error) {
            fprintf(stderr, "error during loading the glyph\n");
            return error;
        }
        for(int y=0; y<face->glyph->bitmap.rows; y++) {
            for(int x=0; x<face->glyph->bitmap.width; x++) {
                int x1 = x0 + x + face->glyph->bitmap_left;
                int y1 = y0 + face->glyph->bitmap_top - y + 1;
                int value = face->glyph->bitmap.buffer[y*face->glyph->bitmap.width+x];
                if(value) {
                    seg(x1, y1) = i+1;
                    gray(x1, y1) = value;
                    onePixel = true;
                }
            }
        }
        x0 += face->glyph->advance.x >> 6;
        y0 += face->glyph->advance.y >> 6;
    }
    if(!onePixel) {
        return -1;
    }
    return 0;
}

template<class T>
void wiggle(narray<T>& dstImg, narray<T>& srcImg, float xRad, float yRad,
        float xAmpl, float yAmpl, float xShift, float yShift, T bg, bool interpolate) {
    dstImg.makelike(srcImg);
    for(int x=0; x<srcImg.dim(0); x++) {
        for(int y=0; y<srcImg.dim(1); y++) {
            float x1 = x + yAmpl * sin((y+yShift) * yRad / srcImg.dim(1));
            float y1 = y + xAmpl * sin((x+xShift) * xRad / srcImg.dim(0));
            if(x1 >= 0 && x1 < srcImg.dim(0) && y1 >= 0 && y1 < srcImg.dim(1)) {
                if(interpolate) {
                    dstImg(x, y) = bilin(srcImg, x1, y1);
                } else {
                    dstImg(x, y) = srcImg((int)x1, (int)y1);
                }
            } else {
                dstImg(x, y) = bg;
            }
        }
    }
}

template<class T>
void wiggle(narray<T>& img, float xRad, float yRad,
        float xAmpl, float yAmpl, float xShift, float yShift, T bg, bool interpolate) {
    narray<T> dstImg;
    dstImg.makelike(img);
    wiggle(dstImg, img, xRad, yRad, xAmpl, yAmpl, xShift, yShift, bg, interpolate);
    img.move(dstImg);
}

template<class T>
void removeCntrl(iustrg<T>& s) {
    iustrg<T> s2;
    for(int i=0; i<s.length(); i++) {
        if(s[i].ord() > 255 || !iscntrl(s[i].ord())) {
            s2.push_back(s[i]);
        }
    }
    s.assign(s2);
}

float uni(float min, float max) {
    return drand48() * (max - min) + min;
}


void drawPage(bytearray& pageImage, intarray& pageSeg, bytearray& gray, intarray& seg, int line, int cPixelHeight) {
    for(int x=0; x<gray.dim(0); x++) {
        for(int y=0; y<gray.dim(1); y++) {
            pageImage(x, pageImage.dim(1) - (cPixelHeight*(line+1)*2) + y) = 255-gray(x, y);
            int value = seg(x, y);
            int l = (value==0 || value==0xFFFFFF)?value:(value | (line << 12));
            pageSeg(x, pageSeg.dim(1) - (cPixelHeight*(line+1)*2) + y) = l;
        }
    }
}

/**
    fixme: too much copying and loops
**/
void writePage(bytearray& pageImage, intarray& pageSeg, int page) {
    if(writePages) {
        bytearray pageImage2;
        intarray pageSeg2;
        pageImage2.copy(pageImage);
        pageSeg2.copy(pageSeg);
        tighten(pageImage2);
        pad_by(pageImage2, 50, 50);
        complement(pageImage2);
        bookStore->putPage(pageImage2, page);
        replace_values(pageSeg2, 0xFFFFFF, 0);
        tighten(pageSeg2);
        pad_by(pageSeg2, 50, 50);
        replace_values(pageSeg2, 0, 0xFFFFFF);
        bookStore->putPage(pageSeg2, page, "cseg.gt");
    }
}

void initFreeType(FT_Library& library, FT_Face& face) {
    if(FT_Init_FreeType(&library)) {
        fprintf(stderr, "error during FreeType initialization\n");
        exit(-1);
    }
    int error = FT_New_Face(library, ttfFile, 0, &face);
    if(error == FT_Err_Unknown_File_Format) {
        fprintf(stderr, "the font file could be opened and read, but it appears that its font format is unsupported\n");
        exit(-1);
    }else if(error) {
        fprintf(stderr, "another error code means that the font file could not be opened or read, or simply that it is broken...\n");
        exit(-1);
    }
    if(FT_Select_Charmap(face, FT_ENCODING_UNICODE)) {
        fprintf(stderr, "error during setting charmap\n");
        exit(-1);
    }
    if(FT_Set_Char_Size(face, cWidth*64, cHeight*64, hDpi, vDpi)) {
        fprintf(stderr, "error during setting the pixel size\n");
        exit(-1);
    }
}

int main(int argc, char* argv[]) {
    try{
        srand48(seed);
        if(!setlocale(LC_CTYPE, "de_DE.UTF-8")) {
            fprintf(stderr, "Can't set the locale!\n");
            exit(-1);
        }
        parseArgs(argc, argv);

        FT_Library library;
        FT_Face face;
        initFreeType(library, face);
        // -- determine approx. character sizes in pixel --
        FT_Load_Char(face, 'M', FT_LOAD_RENDER);
        int cPixelWidth = face->glyph->bitmap.width;
        int cPixelHeight = face->glyph->bitmap.rows;
        int x0 = 0;
        int y0 = cHeight * 2;
        // -- create output directory --
        mode_t dirMode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
        mkdir(outDir, dirMode);
        if(bookName.empty()) {
            sprintf(bookName, "%s-%s-%d-%d_jm%.2f_js%.2f", face->family_name, face->style_name, cWidth, hDpi, jitter_mean, jitter_sigma);
        }
        outDir += "/" + bookName;
        mkdir(outDir, dirMode);

        ocropus::init_ocropus_components();
        make_component(bookStore, "OldBookStore");
        bookStore->setPrefix(outDir);

        autodel<ocropus::ICleanupGray> degrade;
        make_component(degrade, "Degradation");
        degrade->pset("jitter_mean", jitter_mean);
        degrade->pset("jitter_sigma", jitter_sigma);
        degrade->pset("sensitivity_mean", sensitivity_mean);
        degrade->pset("sensitivity_sigma", sensitivity_sigma);
        degrade->pset("threshold_mean", threshold_mean);
        degrade->pset("threshold_sigma", threshold_sigma);

        FILE* logFile = fopen(outDir + "/" + logFilename, "w");
        printArgs(logFile);

        int k = 0;
        int page = 0;
        int line = 0;
        bytearray pageImage(cPixelWidth * 100 * 2, cPixelHeight * nLines * 3);
        intarray pageSeg(cPixelWidth * 100 * 2, cPixelHeight * nLines * 3);
        for(int inputIndex = 0; inputIndex<inputFilenames.length(); inputIndex++) {
            FILE* inputFile = fopen(inputFilenames[inputIndex], "r");
            if(inputFile == NULL) {
                fprintf(stderr, "could not open input file '%s': %s\n", inputFilenames[inputIndex].c_str(), strerror(errno));
                continue;
            }
            fprintf(logFile, "\nfile: %s\n\n", inputFilenames[inputIndex].c_str());
            while(!feof(inputFile)) {
                ustrg string;
                fgetsUTF8(string, inputFile);
                // -- page break --
                if(k % nLines == 0) {
                    if(line > 0) {
                        writePage(pageImage, pageSeg, page);
                    }
                    page++;
                    pageImage.fill(0);
                    pageSeg.fill(0);
                }
                removeCntrl(string);
                // -- skip emtpy lines --
                if(string.empty()) {
                    k++;
                    continue;
                }
                line = (k%nLines) + 1;

                intarray seg(x0 + cPixelWidth * string.length() * 2, y0 + cPixelHeight * 2);
                bytearray gray(seg.dim(0), seg.dim(1));
                fill(seg, 0);
                fill(gray, 0);
                if(renderText(seg, gray, string, 10, 25, face) != 0) {
                    k++;
                    continue;
                }
                tighten(seg);
                tighten(gray);
                pad_by(seg, cPixelWidth/2, cPixelHeight/2);
                pad_by(gray, cPixelWidth/2, cPixelHeight/2);
                complement(gray);
                if(verbose) {
                    bookStore->putLine(gray, page, line, "perfect");
                }
                float xRad = uni(min_xRad, max_xRad);
                float yRad = uni(min_yRad, max_yRad);
                float xAmpl = uni(min_xAmpl, max_xAmpl);
                float yAmpl = uni(min_yAmpl, max_yAmpl);
                float xShift = uni(0.0, gray.dim(0));
                float yShift = uni(0.0, gray.dim(1));

                wiggle(gray, xRad, yRad, xAmpl, yAmpl, xShift, yShift, (unsigned char)0xFF, true);
                wiggle(seg, xRad, yRad, xAmpl, yAmpl, xShift, yShift, 0, false);

                propagate_labels(seg);

                bytearray gray2;
                degrade->cleanup(gray2, gray);
                gray.move(gray2);

                intarray segDide(seg.dim(0), seg.dim(1));
                for(int i=0; i<seg.length1d(); i++) {
                    if(gray.at1d(i) < 200 ) {
                        segDide.at1d(i) = seg.at1d(i);
                    } else {
                        segDide.at1d(i) = 0xFFFFFF;
                    }
                }

                if(writePages) {
                    drawPage(pageImage, pageSeg, gray, segDide, line, cPixelHeight);
                }
                bookStore->putLine(gray, page, line);
                bookStore->putLine(segDide, page, line, "cseg.gt");
                bookStore->putLine(string, page, line, "gt");

                fwriteUTF8(string, logFile);
                fprintf(logFile, "\n");
                k++;
            }
            fclose(inputFile);
        }
        writePage(pageImage, pageSeg, page);
        fclose(logFile);

    } catch(const char* e) {
        fprintf(stderr, "%s\n", e);
    }
    return 0;
}


/**
 * prints the usage, available parameters, .. of the program
 */
void printUsage(int argc, char *argv[]) {
    printf("reads text from stdin or files and writes line segmentation images\n");
    printf("Usage: %s [OPTION...] [FILE]\n\n", argv[0]);
    printf(" Options:\n\n");
    printf("  -o, --output=DIRECTORY          output directory (default: %s)\n", outDir.c_str());
    printf("  -b, --book=NAME                 book name used as top level directory name (default: generated)\n");
    printf("  -p, --pages                     create page images\n");
    printf("  -f, --font=FILENAME             font file (default: %s)\n", ttfFile.c_str());
    printf("  -w, --width=POINTS              chracter width in points (default: %d)\n", cWidth);
    printf("  -H, --height=POINTS             chracter height in points (default: same as width)\n");
    printf("  -d, --dpi=POINTS                dots per inch (default: %d)\n", hDpi);
    printf("  -l, --lines=VALUE               number of lines per page (default: %d)\n", nLines);
    printf("  -j  --jitter_mean=VALUE         didegrade (default: %.3f)\n", jitter_mean);
    printf("  -J  --jitter_sigma=VALUE        didegrade (default: %.3f)\n", jitter_sigma);
    printf("  -s  --sensitivity_mean=VALUE    didegrade (default: %.3f)\n", sensitivity_mean);
    printf("  -S  --sensitivity_sigma=VALUE   didegrade (default: %.3f)\n", sensitivity_sigma);
    printf("  -t  --threshold_mean=VALUE      didegrade (default: %.3f)\n", threshold_mean);
    printf("  -T  --threshold_sigma=VALUE     didegrade (default: %.3f)\n", threshold_sigma);
    printf("  -L, --log=FILENAME              name of log file (default: %s)\n", logFilename.c_str());
    printf("  -v, --verbose                   show verbose outputs\n");
    printf("  -h, --help                      print this message and exit\n");
    printf("\n");
}

void printArgs(FILE* file) {
    time_t t = time(NULL);
    fprintf(file, "date: %s\n", ctime(&t));
    fprintf(file, "output: %s\n", outDir.c_str());
    fprintf(file, "book: %s\n", bookName.c_str());
    fprintf(file, "pages: %s\n", writePages?"yes":"no");
    fprintf(file, "font: %s\n", ttfFile.c_str());
    fprintf(file, "width: %d\n", cWidth);
    fprintf(file, "height: %d\n", cHeight);
    fprintf(file, "dpi: %d\n", hDpi);
    fprintf(file, "lines: %d\n", nLines);
    fprintf(file, "jitter_mean: %.3f\n", jitter_mean);
    fprintf(file, "jitter_sigma: %.3f\n", jitter_sigma);
    fprintf(file, "sensitivity_mean: %.3f\n", sensitivity_mean);
    fprintf(file, "sensitivity_sigma: %.3f\n", sensitivity_sigma);
    fprintf(file, "threshold_mean: %.3f\n", threshold_mean);
    fprintf(file, "threshold_sigma: %.3f\n", threshold_sigma);
    fprintf(file, "seed: %d\n", seed);

    fprintf(file, "min_xRad: %f\n",min_xRad);
    fprintf(file, "max_xRad: %f\n",max_xRad);
    fprintf(file, "min_yRad: %f\n",min_yRad);
    fprintf(file, "max_yRad: %f\n",max_yRad);
    fprintf(file, "min_xAmpl: %f\n",min_xAmpl);
    fprintf(file, "max_xAmpl: %f\n",max_xAmpl);
    fprintf(file, "min_yAmpl: %f\n",min_yAmpl);
    fprintf(file, "max_yAmpl: %f\n",max_yAmpl);
    fprintf(file, "\n\n");
}

/**
 * parses program arguments
 */
void parseArgs(int argc, char *argv[]) {

    static struct option long_options[] = {
        {"output", required_argument, 0, 'o'},
        {"book", required_argument, 0, 'b'},
        {"pages", no_argument, 0, 'p'},
        {"font", required_argument, 0, 'f'},
        {"width", required_argument, 0, 'w'},
        {"height", required_argument, 0, 'H'},
        {"dpi", required_argument, 0, 'd'},
        {"lines", required_argument, 0, 'l'},
        {"jitter_mean", required_argument, 0, 'j'},
        {"jitter_sigma", required_argument, 0, 'J'},
        {"sensitivity_mean", required_argument, 0, 's'},
        {"sensitivity_sigma", required_argument, 0, 'S'},
        {"threshold_mean", required_argument, 0, 't'},
        {"threshold_sigma", required_argument, 0, 'T'},
        {"log", required_argument, 0, 'L'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    int c;
    int option_index = 0;
    while ((c = getopt_long(argc, argv, "o:b:f:w:H:d:l:j:J:s:S:t:T:L:hvp",long_options, &option_index)) != -1) {
        switch (c) {
        case 'o':
            outDir = optarg;
            break;
        case 'b':
            bookName = optarg;
            break;
        case 'p':
            writePages = true;
            break;
        case 'f':
            ttfFile = optarg;
            break;
        case 'w':
            cWidth = atoi(optarg);
            break;
        case 'H':
            cHeight = atoi(optarg);
            break;
        case 'd':
            hDpi = atoi(optarg);
            break;
        case 'l':
            nLines = atoi(optarg);
            break;
        case 'j':
            jitter_mean = atof(optarg);
            break;
        case 'J':
            jitter_sigma = atof(optarg);
            break;
        case 's':
            sensitivity_mean = atof(optarg);
            break;
        case 'S':
            sensitivity_sigma = atof(optarg);
            break;
        case 't':
            threshold_mean = atof(optarg);
            break;
        case 'T':
            threshold_sigma = atof(optarg);
            break;
        case 'L':
            logFilename = optarg;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'h':
            printUsage(argc, argv);
            exit(EXIT_SUCCESS);
            break;
        }
    }
    if(optind == argc) {
        inputFilenames.push("/dev/stdin");
    } else {
        for(int index = optind; index < argc; index++) {
            inputFilenames.push(argv[index]);
        }
    }
    if(cHeight == 0) {
        cHeight = cWidth;
    }
    vDpi = hDpi;
}
