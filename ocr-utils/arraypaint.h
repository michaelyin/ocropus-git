#ifndef arraypaint_h_
#define arraypaint_h_

namespace ocropus {
    // TODO implement these using function templates --tmb
    // TODO create a bunch of narray drawing functions in iulib/utils --tmb
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

    void paint_rectangles(intarray &image,rectarray &rectangles);

}

#endif
