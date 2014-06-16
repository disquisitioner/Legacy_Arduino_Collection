#ifndef _COLORSPACE_H
#define _COLORSPACE_H
typedef struct RGB {
  int r;
  int g;
  int b;
} RGB;

typedef struct RGBcolor {
  float r;
  float g;
  float b;
} RGBcolor;

/* modified from Alvy Ray Smith's site: http://www.alvyray.com/Papers/hsv2rgb.htm */
// H is given on [0.0, 6.0]. S and V are given on [0.0, 1.0].
RGB HSVcolor_to_RGB(float h, float s, float v);
  
#endif
