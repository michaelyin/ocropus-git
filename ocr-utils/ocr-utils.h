// -*- C++ -*-

// Copyright 2006 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz
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
// Project:
// File: ocr-utils.h
// Purpose: miscelaneous routines
// Responsible: mezhirov
// Reviewer:
// Primary Repository:
// Web Sites: www.iupr.org, www.dfki.de


#ifndef h_utils_
#define h_utils_


#include "ocropus.h"

namespace ocropus {
    using namespace colib;

    void throw_fmt(const char *format, ...);

    /// FIXME move into imglib Simply 255 - array.
    void invert(bytearray &a);

    void crop_masked(bytearray &result,
                     bytearray &source,
                     rectangle box,
                     bytearray &mask,
                     int default_value, // where mask is false, also for padding
                     int padding = 0);

    // FIXME this should probably have a border size argument --tmb
    int average_on_border(bytearray &a);

    // Note that black and white are not mutually exclusive. FIXME?
    inline bool background_seems_black(bytearray &a) {
        return average_on_border(a) <= (min(a) + max(a) / 2);
    }
    inline bool background_seems_white(bytearray &a) {
        return average_on_border(a) >= (min(a) + max(a) / 2);
    }
    void optional_check_background_is_darker(bytearray &a);
    void optional_check_background_is_lighter(bytearray &a);

    inline void make_background_white(bytearray &a) {
        if(!background_seems_white(a))
            invert(a);
    }
    inline void make_background_black(bytearray &a) {
        if(!background_seems_black(a))
            invert(a);
    }

    // FIXME get rid of this; this is already defined in narray_ops
    template<class T>
    void mul(narray<T> &a, T coef) {
        for(int i = 0; i < a.length1d(); i++)
            a.at1d(i) *= coef;
    }

    // this is already defined in narray... shift_by --tmb
    // not quite, shift_by fills the background, and this leaves it intact --IM
    /// Copy the `src' to `dest',
    /// moving it by `shift_x' to the right and by `shift_y' up.
    /// The image must fit.
    void blit2d(bytearray &dest,
                const bytearray &src,
                int shift_x = 0,
                int shift_y = 0);

    // FIXME move into narray-util
    float median(intarray &a);

    // FIXME explain what method this uses --tmb
    /// Estimate xheight given a slope and a segmentation.
    /// (That's an algorithm formerly used together with MLP).
    float estimate_xheight(intarray &seg, float slope);

    void plot_hist(FILE *stream, floatarray &hist);

    // FIXME implement these using function templates
    // FIXME create a bunch of narray drawing functions in iulib/utils
    /// Draw a rectangle on an image
    void paint_box(intarray &image, rectangle r,
                   int color, bool inverted=false);
    void paint_box(bytearray &image, rectangle r,
                   byte color, bool inverted=false);
    void paint_box_border(intarray &image, rectangle r,
                          int color, bool inverted=false);
    void paint_box_border(bytearray &image, rectangle r,
                          byte color, bool inverted=false);

    // Draw an array of rectangle boundaries into a new image
    void draw_rects(intarray &out, bytearray &in,
                    narray<rectangle> &rects,
                    int downsample_factor=1, int color=0x00ff0000);

    // Draw an array of filled rectangles into a new image
    void draw_filled_rects(intarray &out, bytearray &in,
                           narray<rectangle> &rects,
                           int downsample_factor=1,
                           int color=0x00ffff00,
                           int border_color=0x0000ff00);

    void get_line_info(float &baseline,
                       float &xheight,
                       float &descender,
                       float &ascender,
                       intarray &seg);

    const char *get_version_string();
    void set_version_string(const char *);

    // FIXME redundant with rowutils.h

    template<class T>
    DEPRECATED void extract_row(narray<T> &row, narray<T> &matrix, int index) {
        ASSERT(matrix.rank() == 1 || matrix.rank() == 2);
        if(matrix.rank() == 2) {
            row.resize(matrix.dim(1));
            for(int i = 0; i < row.length(); i++)
                row[i] = matrix(index, i);
        } else {
            row.resize(1);
            row[0] = matrix[index];
        }
    }

    // FIXME redundant with rowutils.h
    // Append a row to the 2D table.

    template<class T>
    DEPRECATED void append_row(narray<T> &table, narray<T> &row);

    template<class T>
    void append_row(narray<T> &table, narray<T> &row) {
        ASSERT(row.length());
        if(!table.length1d()) {
            copy(table, row);
            table.reshape(1, table.length());
            return;
        }
        int h = table.dim(0);
        int w = table.dim(1);
        ASSERT(row.length() == w);
        table.reshape(table.total);
        table.grow_to(table.total + w);
        table.reshape(h + 1, w);
        for(int i = 0; i < row.length(); i++)
            table(h, i) = row[i];
    }

    // FIXME move into narray-util --tmb

    template<class T>
    DEPRECATED bool is_nan_free(T &v) {
        for(int i=0;i<v.length1d();i++)
            if(isnan(v.at1d(i)))
                return false;
        return true;
    }

    /// Extract the set of pixels with the given value and return it
    /// as a black-on-white image.
    inline void extract_segment(bytearray &result,
                                intarray &image,
                                int n) {
        makelike(result, image);
        fill(result, 255);
        for(int i = 0; i < image.length1d(); i++) {
            if(image.at1d(i) == n)
                result.at1d(i) = 0;
        }
    }

    // remove small connected components (really need to add more general marker code
    // to the library)
    template <class T>
    void remove_small_components(narray<T> &bimage,int mw,int mh);
    template <class T>
    void remove_marginal_components(narray<T> &bimage,int x0,int y0,int x1,int y1);

    /// Split a string into a list using the given array of delimiters.
    void split_string(narray<strbuf> &components,
                      const char *path,
                      const char *delimiters);
    int binarize_simple(bytearray &result, bytearray &image);
    int binarize_simple(bytearray &image);
    void binarize_with_threshold(bytearray &out, bytearray &in, int threshold);
    void binarize_with_threshold(bytearray &in, int threshold);

    void runlength_histogram(floatarray &hist, bytearray &img, rectangle box,
                             bool white=false,bool vert=false);
    inline void runlength_histogram(floatarray &hist, bytearray &image,
                             bool white=false,bool vert=false) {
        runlength_histogram(hist,image,rectangle(0,0,image.dim(0),image.dim(1)),white,vert);
    }

    // why is this a public function? make local function somewhere. --tmb
    // this is a public function because it's used from a Lua script,
    // namely heading detector. -- IM
    int find_median_in_histogram(floatarray &);

    // this function should not be used as part of regular software; the
    // layout analysis should yield "cleans" text lines.  --tmb
    // This function is sometimes useful when you need to deal with a dataset
    // that was not put through our layout analysis. --IM
    void remove_neighbour_line_components(bytearray &line);

    void paint_rectangles(intarray &image,rectarray &rectangles);

    void strbuf_format(strbuf &str, const char *format, ...);
    void code_to_strbuf(strbuf &s, int code);

    template<class T>
    void rotate_90(narray<T> &out, narray<T> &in);
    template<class T>
    void rotate_180(narray<T> &out, narray<T> &in);
    template<class T>
    void rotate_270(narray<T> &out, narray<T> &in);
}

#endif
