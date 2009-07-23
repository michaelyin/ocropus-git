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
// Project: mnist-tools
// File: cseg2mnist.cc
// Purpose: create mnist files out of character segmented images
// Responsible: remat
// Reviewer:
// Primary Repository: https://svn.iupr.org/hacks/remat/mnist-tools
// Web Sites: www.iupr.org, www.dfki.de

#include <glob.h>
#include <getopt.h>

#include <iulib.h>
#include <colib.h>
#include <ocropus.h>

#include "mnist.h"

static int maxSize = 255;
static int minSize = 5;
static float sizeRate = 0.80;
static int targetWidth = 32;
static int targetHeight = 32;
static int padX = 2;
static int padY = 2;
static unsigned char fgColor = 255;
static unsigned char bgColor = 0;
static strg outDir = ".";
static strg prefix = "cseg2mnist";
static objlist<strg> files;
static bool verbose = false;
static int bookNo = 0;

void printUsage(int argc, char* argv[]) {
    printf("Usage:\n\t%s [options] <segmentation-files>\n\n", argv[0]);
    printf("Options:\n");
    printf("\t-o, --output=DIRECTORY   output folder for mnist files (default: %s)\n", outDir.c_str());
    printf("\t-P, --prefix=STRING      prefix for output files (default: %s)\n", prefix.c_str());
    printf("\t-B, --book=NUMBER        book number for attributes (default: %d)\n", bookNo);
    printf("\t-m, --min=PIXEL          minimal character size in pixel (default: %d)\n", minSize);
    printf("\t-x, --max=PIXEL          maximal character size in pixel (default: %d)\n", maxSize);
    printf("\t-r, --rate=RATE          rate of characters that have to fit the maximal size (default: %lf)\n", sizeRate);
    printf("\t-w, --width=PIXEL        final character size in pixel without padding (default: %d)\n", targetWidth);
    printf("\t-H, --height=PIXEL       final character size in pixel without padding (default: %d)\n", targetHeight);
    printf("\t-p, --pad=PIXEL          additional padding (default: %d)\n", padX);
    printf("\t-f, --fg=COLOR           foreground color 0..255 (default: %d)\n", fgColor);
    printf("\t-b, --bg=COLOR           background color 0..255 (default: %d)\n", bgColor);
    printf("\t-v, --verbose            print additional information\n");
    printf("\t-h, --help               print this messsage and exit\n");
}

void parseArgs(int argc, char *argv[]) {
    if(argc < 2) {
        printf("too few arguments\n");
        printUsage(argc, argv);
        exit(-1);
    }
    static struct option long_options[] = {
        {"output", required_argument, 0, 'o'},
        {"prefix", required_argument, 0, 'P'},
        {"book", required_argument, 0, 'B'},
        {"min", required_argument, 0, 'm'},
        {"max", required_argument, 0, 'x'},
        {"rate", required_argument, 0, 'r'},
        {"width", required_argument, 0, 'w'},
        {"height", required_argument, 0, 'H'},
        {"pad", required_argument, 0, 'p'},
        {"fg", required_argument, 0, 'f'},
        {"bf", required_argument, 0, 'b'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    int c;
    int option_index = 0;
    while ((c = getopt_long(argc, argv, ":hvo:m:x:r:w:H:p:f:b:P:B:",long_options, &option_index)) != -1) {
        switch (c) {
        case 'h':
            printUsage(argc, argv);
            exit(EXIT_SUCCESS);
            break;
        case 'v':
            verbose = true;
            break;
        case 'o':
            outDir.assign(optarg);
            break;
        case 'P':
            prefix.assign(optarg);
            break;
        case 'B':
            bookNo = atoi(optarg);
            break;
        case 'm':
            minSize = atoi(optarg);
            break;
        case 'x':
            maxSize = atoi(optarg);
            break;
        case 'r':
            sizeRate = atoi(optarg);
            break;
        case 'w':
            targetWidth = atoi(optarg);
            break;
        case 'H':
            targetHeight = atoi(optarg);
            break;
        case 'p':
            padX = padY = atoi(optarg);
            break;
        case 'f':
            fgColor = atoi(optarg);
            break;
        case 'b':
            bgColor = atoi(optarg);
            break;
        }
    }
    while(optind < argc) {
        files.push().assign(argv[optind++]);
    }
}

struct Glob {
    glob_t g;
    Glob(const char *pattern,int flags=0) {
        glob(pattern,flags,0,&g);
    }
    ~Glob() {
        globfree(&g);
    }
    int length() {
        return g.gl_pathc;
    }
    const char *operator()(int i) {
        CHECK(i>=0 && i<length());
        return g.gl_pathv[i];
    }
};

template<class T>
void extractSubimageOwn(narray<T>& dstImage, intarray& seg, int x0, int y0, int x1, int y1, int i) {
    dstImage.resize(x1-x0, y1-y0);
    for(int x=0; x<dstImage.dim(0); x++) {
        for(int y=0; y<dstImage.dim(1); y++) {
            if(seg.at(x0+x, y0+y) == i) {
                dstImage(x, y) = fgColor;
            } else {
                dstImage(x, y) = bgColor;
            }
        }
    }
}


bool isBg(intarray& seg, int x0, int y0, int i, int k=2) {
    if(x0 < 0 || x0 >= seg.dim(0) || y0 < 0 || y0 >= seg.dim(1)) {
        return true;
    }
    bool bg = false;
    for(int x=x0-k; x<=x0+k && !bg; x++) {
        for(int y=y0-k; y<=y0+k && !bg; y++) {
            if(x>=0 && x<seg.dim(0) && y>=0 && y<seg.dim(1) && seg(x, y) > 0 && seg(x, y) != i) {
                bg = true;
            }
        }
    }
   return bg;
}

template<class T>
void extractSubimageOwn(narray<T>& dstImage, narray<T>& srcImage, intarray& seg, int x0, int y0, int x1, int y1, int i, int bg) {
    dstImage.resize(x1-x0, y1-y0);
    for(int x=0; x<dstImage.dim(0); x++) {
        for(int y=0; y<dstImage.dim(1); y++) {
            int xx = x0+x;
            int yy = y0+y;
            //if(xx < 0 || xx >= srcImage.dim(0) || yy < 0 || yy >= srcImage.dim(1) || (seg.at(xx, yy) > 0 && seg.at(xx, yy) != i)) {
            if(isBg(seg, xx, yy,  i)) {
                dstImage(x, y) = bg;
            } else {
                dstImage(x, y) = srcImage(xx, yy);
            }

        }
    }
}

// -- returns the centroid of an image --
point centroid(bytearray& image) {
    point mean(0, 0);
    int sum = 0;
    for(int x=0; x<image.dim(0); x++) {
        for(int y=0; y<image.dim(1); y++) {
            mean.x += image(x,y) * x;
            mean.y += image(x,y) * y;
            sum += image(x,y);
        }
    }
    mean.x /= sum;
    mean.y /= sum;
    return mean;
}

// -- shifts the centroid of an image to it's center --
// -- may resizes the image, but as less as needed --
void shiftCentroid(bytearray& image, rectangle& box) {
    // -- pad by image size to ensure that we don't lose anything at shifting --
    int pX = image.dim(0);
    int pY = image.dim(1);
    pad_by(image, pX, pY, bgColor);
    box.pad_by(pX, pY);
    // -- shift centroid to the new center --
    point mean = centroid(image);

    int diffX = (int)floor((image.dim(0)/2.0 - mean.x)+0.5);
    int diffY = (int)floor((image.dim(1)/2.0 - mean.y)+0.5);
    if(abs(diffX) > pX || abs(diffY) > pY) {
        printf("WARNING: padding too small\n");
    }

    shift_by(image, diffX, diffY, bgColor);
    box.shift_by(-diffX, -diffY);
    // -- determine minimal size of the image keeping the centroid at the center --
    int x0 = image.dim(0);
    int x1 = 0;
    int y0 = image.dim(1);
    int y1 = 0;
    int xS = 0;//MAX(pX + diffX - 1, 0);
    int xE = image.dim(0) - 1;//MIN(image.dim(0) - xS + 2, image.dim(0)-1);
    int yS = 0;//MAX(pY + diffY - 1, 0);
    int yE = image.dim(1) - 1;//MIN(image.dim(1) - yS + 2, image.dim(1)-1);

    for(int x=xS; x<=xE; x++) {
        for(int y=yS; y<=yE; y++) {
            if(image(x,y) != bgColor) {
                x0 = min(x0, x);
                y0 = min(y0, y);
                x1 = max(x1, x);
                y1 = max(y1, y);
            }
        }
    }
    int x0Final = min(x0, image.dim(0) - x1 - 1);
    int y0Final = min(y0, image.dim(1) - y1 - 1);
    int x1Final = max(x1, image.dim(0) - x0 - 1);
    int y1Final = max(y1, image.dim(1) - y0 - 1);
    // -- extract minimal image with centered centroid --
    bytearray image2;
    extract_subimage(image2, image, x0Final, y0Final, x1Final + 1, y1Final + 1);
    box.x1 = box.x0 + x1Final + 1;
    box.y1 = box.y0 + y1Final + 1;
    box.x0 += x0Final;
    box.y0 += y0Final;
    image.move(image2);
}


// -- returns minimal size that fits given number of images --
// -- not optimal, because it handles width and height separately --
point optimalSize(objlist<bytearray>& images, int maxSize, float rate) {
    // -- create histogram for widths and heights --
    intarray widths(maxSize*3 + 3);
    intarray heights(maxSize*3 + 3);
    fill(widths, 0);
    fill(heights, 0);
    for(int i=0; i<images.length(); i++) {
        widths[images[i].dim(0)] += 1;
        heights[images[i].dim(1)] += 1;
    }
    point opt(0,0);
    float n = (float)images.length();
    // -- determine minimal size --
    int w = 0;
    int h = 0;
    for(int i=1; i<maxSize; i++) {
        w += widths[i];
        h += heights[i];
        if(w / n < rate) {
            opt.x = i+1;
        }
        if(h / n < rate) {
            opt.y = i+1;
        }
        if(w / n >= rate && h / n >= rate) {
            break;
        }
    }
    return opt;
}


// -- adjust (increase) the optimal size to fit a defined aspect ratio --
point adjustOptimalSize(point& size, float aspect) {
    float currAspect = size.y / (float)size.x;
    point newSize(size);
    if(aspect > 1.0) {
        if(currAspect > 1.0) {
            newSize.x = (int)floor(size.y * aspect);
        } else {
            newSize.x = (int)floor(size.y * aspect);
        }
    } else {
        if(currAspect > 1.0) {
            newSize.x = (int)floor(size.y / aspect);
        } else {
            newSize.y = (int)floor(size.x / aspect);
        }
    }
    return newSize;
}

template<class T>
void adjustSize(narray<T>& img, point& size, rectangle& box, T bg = (T)bgColor) {
    int pX = (size.x - img.dim(0)) / 2;
    int pY = (size.y - img.dim(1)) / 2;
    pad_by(img, pX, pY, bg);
    box.pad_by(pX, pY);
    narray<T> img2;
    scale_interpolate(img2, img, targetWidth, targetHeight);
    pad_by(img2, padX, padY, bg);
    img.move(img2);
}

template<class T>
void removeSpaces(iustrg<T>& string) {
    iustrg<T> result;
    for(int k=0; k<string.length(); k++) {
        if(string[k] != ' ') {
            result.push_back(string[k]);
        }
    }
    string.assign(result);
}

void rgb2gray(bytearray& dst, intarray& src) {
    dst.makelike(src);
    for(int i=0; i<src.length1d(); i++) {
        //float r = (src.at1d(i) >> 16) & 0xFF;
        //float g = (src.at1d(i) >> 8) & 0xFF;
        //float b = src.at1d(i) & 0xFF;
        //dst.at1d(i) = 0.299*r + 0.587*g + 0.114*b;
        dst.at1d(i) = (((src.at1d(i) >> 16) & 0xFF) + ((src.at1d(i) >> 8) & 0xFF) + (src.at1d(i) & 0xFF)) / 3;
    }
}

template<class T, class S>
T mostCommonBG(narray<T>& img, narray<S>& mask) {
    inthash<int> c;
    int maxI = 0;
    int maxV = -1;
    for(int k=0; k<img.length1d(); k++) {
        if(mask.at1d(k) == 0 && img.at1d(k) != -1) {
            int* x = c.find(img.at1d(k));
            if(x == NULL) {
                c(img.at1d(k)) = 0;
                x = c.find(img.at1d(k));
            }
            *x = *x+1;
            if(*x > maxV) {
                maxV = *x;
                maxI = img.at1d(k);
            }
        }
    }
    return maxI;
}

int main(int argc, char* argv[]) {
    if(sizeof(float) != 4) {
        fprintf(stderr, "float not 4 bytes: %lu\n", sizeof(float));
        exit(-1);
    }

    parseArgs(argc, argv);

    fprintf(stderr, "maxSize: %d\n", maxSize);
    fprintf(stderr, "minSize: %d\n", minSize);
    fprintf(stderr, "sizeRate: %f\n", sizeRate);
    fprintf(stderr, "targetWidth: %d\n", targetWidth);
    fprintf(stderr, "targetHeight: %d\n", targetHeight);
    fprintf(stderr, "padX: %d\n", padX);
    fprintf(stderr, "padY: %d\n", padY);
    fprintf(stderr, "bookNo: %d\n", bookNo);

    objlist<bytearray> charImages;
    objlist<intarray> lineImages, lineSegs;
    strg chars;
    floatarray costs;
    intarray lineNos, pageNos, charNos, index;
    rectarray origBBoxes, newBBoxes;

    rectarray bboxes;
    ustrg text;

    for(int j=0; j<files.length(); j++) {
        printf("processing \"%s\": 0%%", files[j].c_str()); fflush(stdout);
        Glob segFiles(files[j]);
        for(int i=0; i<segFiles.length(); i++) {
            intarray& seg = lineSegs.push();
            intarray& color = lineImages.push();
            read_image_packed(seg, segFiles(i));
            strg colorFilename = segFiles(i);
            colorFilename = colorFilename.erase(colorFilename.length()-8)+"png";
            read_image_packed(color, colorFilename);
            strg textFilename = segFiles(i);
            textFilename = textFilename.erase(textFilename.length()-8)+"gt.txt";
            strg costsFilename = segFiles(i);
            costsFilename = costsFilename.erase(costsFilename.length()-8)+"costs";
            fprintf(stderr, "%s\n%s\n%s\n", segFiles(i), textFilename.c_str(), costsFilename.c_str());

            strg ns =textFilename.substr(textFilename.length()-16, 9);
            float pageNo, lineNo;
            sscanf(ns.c_str(), "%f/%f", &pageNo, &lineNo);

            FILE* textFile = fopen(textFilename, "r");
            fgetsUTF8(text, textFile);
            fclose(textFile);
            removeSpaces(text);

            replace_values(seg, 0xFFFFFF, 0);
            bounding_boxes(bboxes, seg);

            if(bboxes.length()-1 != text.length()) {
                fprintf(stderr, "number of bbounding boxes doesn't match text length: %d     %d\n", bboxes.length()-1, text.length());
                for(int j=0; j<text.length(); j++) {
                    fprintf(stderr, "%c", text[j].ord());
                }
                fprintf(stderr, "\n");
                continue;
            }

            FILE* costsFile = fopen(costsFilename, "r");
            int dummy;
            float cost;
            fscanf(costsFile, "%d %f", &dummy, &cost); // -- skip first line --

            for(int t=0; t<text.length(); t++) {
                if(text[t] > 127) {
                    fprintf(stderr, "skip character > 127 (%d)\n", text[t].ord());
                    continue;
                }
                rectangle& box = bboxes[t+1];
                if(box.width() < minSize || box.width() > maxSize || box.height() < minSize || box.height() > maxSize) {
                    fprintf(stderr, "skip character (%d, %d)\n", box.width(), box.height());
                    continue;
                }

                bytearray& charImage = charImages.push();
                extractSubimageOwn(charImage, seg, box.x0, box.y0, box.x1, box.y1, t+1);

                chars.push_back(text[t].ord());

                rectangle b2(box);
                shiftCentroid(charImage, b2);

                fscanf(costsFile, "%d %f", &dummy, &cost);
                costs.push(cost);
                pageNos.push(pageNo);
                lineNos.push(lineNo);
                charNos.push(t+1);
                origBBoxes.push(box);
                newBBoxes.push(b2);
                index.push(lineImages.length()-1);
            }
            fclose(costsFile);
            fprintf(stderr, "OK\n");
            if(i%100 == 0) {
                printf("\rprocessing \"%s\": %0.2f%%", files[j].c_str(), i*100/(float)segFiles.length()); fflush(stdout);
            }
        }
        printf("\rprocessing \"%s\": 100.00%%\n", files[j].c_str());
    }

    point size1 = optimalSize(charImages, maxSize, sizeRate);
    point size = adjustOptimalSize(size1, targetWidth/(float)targetHeight);

    strg mnistImagesFile = outDir + "/" + prefix + "-images-idx3-ubyte";
    strg mnistGrayImagesFile = outDir + "/" + prefix + "-images-gray-idx3-ubyte";
    strg mnistColorImagesFile = outDir + "/" + prefix + "-images-rgb-idx3-ubyte";
    strg mnistLabelsFile = outDir + "/" + prefix + "-labels-idx1-ubyte";
    strg mnistAttributesFile = outDir + "/" + prefix + "-attributes-idx1-ubyte";
    bytearray labels;
    FILE* mnistImages = openMnist(mnistImagesFile, 3);
    FILE* mnistGrayImages = openMnist(mnistGrayImagesFile, 3);
    FILE* mnistColorImages = openMnist(mnistColorImagesFile, 4);
    FILE* mnistLabels = openMnist(mnistLabelsFile, 1);
    FILE* mnistAttributes = openMnist(mnistAttributesFile, 2, 0x0D);
    writeMnist(targetHeight + 2*padY, mnistImages);
    writeMnist(targetWidth + 2*padX, mnistImages);
    writeMnist(targetHeight + 2*padY, mnistGrayImages);
    writeMnist(targetWidth + 2*padX, mnistGrayImages);
    writeMnist(targetHeight + 2*padY, mnistColorImages);
    writeMnist(targetWidth + 2*padX, mnistColorImages);
    writeMnist(3, mnistColorImages); // -- additional dimension with length 3 for color images (rgb)
    writeMnist(7, mnistAttributes);  // -- 7 = cost + bookNo + pageNo + lineNo + charNo + orig width + orig height --


    printf("%d characters\n", charImages.length());
    int n = 0;
    // -- rescale images and push them into the mnist files --
    printf("writing to MNIST files: 0%%"); fflush(stdout);
    for(int i=0; i<charImages.length(); i++) {
        bytearray& charImage = charImages(i);
        rectangle& box = newBBoxes(i);
        intarray& lineImg = lineImages(index(i));
        intarray& lineSeg = lineSegs(index(i));
        if(charImage.dim(0) <= size.x && charImage.dim(1) <= size.y && chars[i] < 256) {
            adjustSize(charImage, size, box);

            intarray charColorImage, charColorImage2;
            extractSubimageOwn(charColorImage, lineImg, lineSeg, box.x0, box.y0, box.x1, box.y1, charNos(i), -1);
            scale_interpolate(charColorImage2, charColorImage, targetWidth, targetHeight);
            charColorImage.move(charColorImage2);
            pad_by(charColorImage, padX, padY, -1);
            replace_values(charColorImage, -1, mostCommonBG(charColorImage, charImage));

            bytearray charGrayImage;
            rgb2gray(charGrayImage, charColorImage);

            writeMnist(charImage, mnistImages);
            writeMnist(charGrayImage, mnistGrayImages);
            writeMnist(charColorImage, mnistColorImages);
            writeMnist(chars[i], mnistLabels);
            writeMnist((float)bookNo, mnistAttributes);
            writeMnist((float)pageNos(i), mnistAttributes);
            writeMnist((float)lineNos(i), mnistAttributes);
            writeMnist((float)charNos(i), mnistAttributes);
            writeMnist(costs(i), mnistAttributes);
            writeMnist((float)origBBoxes(i).width(), mnistAttributes);
            writeMnist((float)origBBoxes(i).height(), mnistAttributes);
            n++;
        }
        if(i%1000 == 0) {
            printf("\rwriting to MNIST files: %0.2f%%", i*100/(float)charImages.length()); fflush(stdout);
        }
    }
    printf("\rwriting to MNIST files: 100.00%%\n");
    // -- update mnist headers with number of records --
    closeMnist(mnistImages, n);
    closeMnist(mnistGrayImages, n);
    closeMnist(mnistColorImages, n);
    closeMnist(mnistLabels, n);
    closeMnist(mnistAttributes, n);
    printf("%d characters\n", n);
    fprintf(stderr, "%d characters\n", n);
}
