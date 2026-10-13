/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --bpp 1 --size 16 --font /Users/mattivesala/Documents/Arduino/ESP32-Crowpanel-compass/UI/squareline_project_v3/assets/ArchivoBlack-Regular.ttf -o /Users/mattivesala/Documents/Arduino/ESP32-Crowpanel-compass/UI/squareline_project_v3/assets/ui_font_FontCelestial16.c --format lvgl -r 0x20-0x7f --symbols ° --no-compress --no-prefilter
 ******************************************************************************/

#include "ui.h"

#ifndef UI_FONT_FONTCELESTIAL16
#define UI_FONT_FONTCELESTIAL16 1
#endif

#if UI_FONT_FONTCELESTIAL16

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xff, 0xff, 0xf8, 0xff, 0x80,

    /* U+0022 "\"" */
    0xff, 0xff, 0xff, 0x48,

    /* U+0023 "#" */
    0x19, 0x86, 0x47, 0xfd, 0xff, 0x33, 0xc, 0xcf,
    0xfb, 0xfe, 0x26, 0x9, 0x86, 0x60,

    /* U+0024 "$" */
    0x18, 0xc, 0xf, 0x9f, 0xfe, 0x3f, 0xe1, 0xfc,
    0x3f, 0xe3, 0xff, 0xdf, 0xc3, 0x1, 0x80, 0xc0,

    /* U+0025 "%" */
    0x78, 0x23, 0xf1, 0x8c, 0xc4, 0x33, 0x20, 0xfd,
    0x81, 0xe4, 0xe0, 0x37, 0xc1, 0x9b, 0x4, 0x6c,
    0x31, 0xb1, 0x83, 0x80,

    /* U+0026 "&" */
    0x3f, 0x83, 0xff, 0x38, 0x39, 0xc0, 0x7, 0xcf,
    0xbe, 0x7f, 0x83, 0x9c, 0x1c, 0xf1, 0xe3, 0xfe,
    0xf, 0xe0,

    /* U+0027 "'" */
    0xff, 0xf4,

    /* U+0028 "(" */
    0x19, 0x9c, 0xee, 0x73, 0x9c, 0xe7, 0x38, 0xe7,
    0x18, 0x60,

    /* U+0029 ")" */
    0xc3, 0x1c, 0xe3, 0x9c, 0xe7, 0x39, 0xce, 0xe7,
    0x33, 0x0,

    /* U+002A "*" */
    0x25, 0x7f, 0xfa, 0x90,

    /* U+002B "+" */
    0x18, 0x18, 0x18, 0x18, 0xff, 0xff, 0x18, 0x18,
    0x18,

    /* U+002C "," */
    0xff, 0xb5, 0x80,

    /* U+002D "-" */
    0xff,

    /* U+002E "." */
    0xff, 0x80,

    /* U+002F "/" */
    0x33, 0x33, 0x36, 0x66, 0x66, 0xcc, 0xc0,

    /* U+0030 "0" */
    0x3e, 0x3f, 0xb8, 0xfc, 0x7e, 0x3f, 0x1f, 0x8f,
    0xc7, 0xe3, 0xbf, 0x8f, 0x80,

    /* U+0031 "1" */
    0x1c, 0xfc, 0xfc, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c,
    0x1c, 0xff, 0xff,

    /* U+0032 "2" */
    0x3e, 0x3f, 0xb8, 0xfc, 0x70, 0x78, 0x78, 0x78,
    0xf0, 0x7f, 0xff, 0xff, 0xe0,

    /* U+0033 "3" */
    0x3e, 0x3f, 0xf8, 0xe0, 0x70, 0xf0, 0x78, 0xf,
    0xc7, 0xe3, 0xbf, 0x8f, 0x80,

    /* U+0034 "4" */
    0x1c, 0xe, 0xc3, 0xb1, 0xdc, 0x67, 0x39, 0xcf,
    0xff, 0xff, 0x7, 0x1, 0xc0, 0x70,

    /* U+0035 "5" */
    0x7f, 0x3f, 0x9f, 0xcd, 0xc7, 0xf3, 0x1c, 0xf,
    0xc7, 0xe3, 0xbf, 0x8f, 0x80,

    /* U+0036 "6" */
    0x3e, 0x3f, 0xf8, 0xff, 0xcf, 0xf7, 0x1f, 0x8f,
    0xc7, 0xe3, 0xbf, 0x8f, 0x80,

    /* U+0037 "7" */
    0xff, 0xff, 0xff, 0xc0, 0xe0, 0xe0, 0xe0, 0x70,
    0x30, 0x38, 0x1c, 0xe, 0x0,

    /* U+0038 "8" */
    0x3e, 0x7f, 0xf8, 0xfc, 0x77, 0xf3, 0xfb, 0x8f,
    0xc7, 0xe3, 0xbf, 0x8f, 0x80,

    /* U+0039 "9" */
    0x3e, 0x3f, 0xb8, 0xfc, 0x7e, 0x3f, 0xfc, 0xff,
    0xc7, 0xe3, 0x3f, 0x8f, 0x80,

    /* U+003A ":" */
    0xff, 0x80, 0x3f, 0xe0,

    /* U+003B ";" */
    0xff, 0x80, 0x3f, 0xed, 0x60,

    /* U+003C "<" */
    0x1, 0x83, 0xcf, 0xdf, 0x8e, 0x7, 0xc0, 0xfc,
    0x1f, 0x1, 0x80,

    /* U+003D "=" */
    0xff, 0xff, 0x0, 0x0, 0xff, 0xff,

    /* U+003E ">" */
    0x80, 0x78, 0x3f, 0x3, 0xf0, 0x78, 0x7d, 0xf9,
    0xe0, 0xc0, 0x0,

    /* U+003F "?" */
    0x3c, 0x7f, 0xe7, 0xe7, 0xf, 0x1e, 0x1c, 0x0,
    0x1c, 0x1c, 0x1c,

    /* U+0040 "@" */
    0x1f, 0x4, 0x11, 0x1, 0x20, 0x19, 0xf3, 0x77,
    0x60, 0xec, 0xfd, 0xbb, 0xb7, 0xfa, 0xf7, 0x20,
    0x2, 0x0, 0x3c, 0x0,

    /* U+0041 "A" */
    0xf, 0x0, 0xf8, 0x1f, 0x81, 0xfc, 0x1d, 0xc3,
    0x9c, 0x39, 0xe7, 0xfe, 0x7f, 0xe7, 0xf, 0xf0,
    0xf0,

    /* U+0042 "B" */
    0xff, 0xbf, 0xfe, 0x1f, 0x87, 0xff, 0xbf, 0xee,
    0x1f, 0x87, 0xe1, 0xff, 0xff, 0xf8,

    /* U+0043 "C" */
    0x1f, 0xf, 0xf9, 0xff, 0xf8, 0xfe, 0xf, 0xc0,
    0x38, 0x3f, 0x8f, 0x7f, 0xef, 0xf8, 0x7e, 0x0,

    /* U+0044 "D" */
    0xfe, 0x3f, 0xef, 0xfb, 0x8f, 0xe1, 0xf8, 0x7e,
    0x1f, 0x8f, 0xff, 0xbf, 0xef, 0xe0,

    /* U+0045 "E" */
    0xff, 0xff, 0xff, 0xff, 0x80, 0xff, 0xbf, 0xee,
    0x3, 0x80, 0xff, 0xff, 0xff, 0xfc,

    /* U+0046 "F" */
    0xff, 0xff, 0xff, 0xfc, 0xe, 0x7, 0xfb, 0xfd,
    0xc0, 0xe0, 0x70, 0x38, 0x0,

    /* U+0047 "G" */
    0x1f, 0xf, 0xf9, 0xff, 0xf8, 0x7e, 0x1, 0xcf,
    0xf9, 0xff, 0x7, 0x71, 0xef, 0xfc, 0x7d, 0x80,

    /* U+0048 "H" */
    0xe0, 0xfc, 0x1f, 0x83, 0xf0, 0x7f, 0xff, 0xff,
    0xff, 0xff, 0x7, 0xe0, 0xfc, 0x1f, 0x83, 0x80,

    /* U+0049 "I" */
    0xff, 0xff, 0xff, 0xff, 0x80,

    /* U+004A "J" */
    0x3, 0x81, 0xc0, 0xe0, 0x70, 0x38, 0x1c, 0xf,
    0xc7, 0xff, 0xbf, 0x8f, 0x80,

    /* U+004B "K" */
    0xe3, 0xfc, 0x7b, 0x9e, 0x77, 0x8f, 0xe1, 0xfe,
    0x3f, 0xe7, 0xbc, 0xe3, 0xdc, 0x7b, 0x87, 0x80,

    /* U+004C "L" */
    0xe0, 0x70, 0x38, 0x1c, 0xe, 0x7, 0x3, 0x81,
    0xc0, 0xff, 0xff, 0xff, 0xe0,

    /* U+004D "M" */
    0xf8, 0xff, 0xc7, 0xff, 0x3f, 0xf9, 0xff, 0xdf,
    0xfe, 0xff, 0xbe, 0xfd, 0xf7, 0xef, 0xbf, 0x39,
    0xf9, 0xce,

    /* U+004E "N" */
    0xe0, 0xfe, 0x1f, 0xe3, 0xfe, 0x7f, 0xef, 0xff,
    0xfb, 0xff, 0x3f, 0xe3, 0xfc, 0x3f, 0x83, 0x80,

    /* U+004F "O" */
    0x1f, 0x87, 0xfe, 0x7f, 0xef, 0xf, 0xe0, 0x7e,
    0x7, 0xe0, 0x7f, 0xf, 0x7f, 0xe7, 0xfe, 0x1f,
    0x80,

    /* U+0050 "P" */
    0xff, 0x7f, 0xb8, 0xfc, 0x7e, 0x3f, 0xfb, 0xf9,
    0xc0, 0xe0, 0x70, 0x38, 0x0,

    /* U+0051 "Q" */
    0x1f, 0x87, 0xfc, 0x7f, 0xef, 0xf, 0xe0, 0x7e,
    0x7, 0xe0, 0x7e, 0x7, 0x70, 0xe7, 0xfe, 0x1f,
    0xc0, 0x3e, 0x1, 0xe0,

    /* U+0052 "R" */
    0xff, 0x3f, 0xee, 0x1f, 0x87, 0xe1, 0xff, 0xef,
    0xf3, 0x9e, 0xe7, 0xb8, 0xfe, 0x1c,

    /* U+0053 "S" */
    0x3f, 0x1f, 0xef, 0x1f, 0xe0, 0xff, 0x1f, 0xf3,
    0xff, 0x8f, 0xe1, 0xdf, 0xe3, 0xf0,

    /* U+0054 "T" */
    0xff, 0xff, 0xff, 0xff, 0x87, 0x0, 0xe0, 0x1c,
    0x3, 0x80, 0x70, 0xe, 0x1, 0xc0, 0x38, 0x0,

    /* U+0055 "U" */
    0xe0, 0xfc, 0x1f, 0x83, 0xf0, 0x7e, 0xf, 0xc1,
    0xf8, 0x3f, 0x8f, 0xff, 0xef, 0xf8, 0x7c, 0x0,

    /* U+0056 "V" */
    0xf0, 0xf7, 0x8f, 0x78, 0xe3, 0x9e, 0x39, 0xe3,
    0xdc, 0x1d, 0xc1, 0xfc, 0x1f, 0x80, 0xf8, 0xf,
    0x0,

    /* U+0057 "W" */
    0xf3, 0xcf, 0xf3, 0xcf, 0x73, 0xce, 0x73, 0xee,
    0x7f, 0xee, 0x7f, 0xfe, 0x3f, 0xfc, 0x3e, 0x7c,
    0x3e, 0x7c, 0x3e, 0x7c, 0x1e, 0x78,

    /* U+0058 "X" */
    0x78, 0xf7, 0xde, 0x3d, 0xc1, 0xfc, 0xf, 0x80,
    0xf8, 0x1f, 0x83, 0xfc, 0x3d, 0xe7, 0x9e, 0xf0,
    0xf0,

    /* U+0059 "Y" */
    0xf1, 0xee, 0x39, 0xef, 0x1d, 0xc1, 0xf0, 0x3e,
    0x3, 0x80, 0x70, 0xe, 0x1, 0xc0, 0x38, 0x0,

    /* U+005A "Z" */
    0x7f, 0xef, 0xfd, 0xff, 0x3, 0xc0, 0xf8, 0x3e,
    0x7, 0x81, 0xe0, 0x7f, 0xff, 0xff, 0xff, 0x80,

    /* U+005B "[" */
    0xff, 0xf9, 0xce, 0x73, 0x9c, 0xe7, 0x39, 0xce,
    0x7f, 0xe0,

    /* U+005C "\\" */
    0xcc, 0xc6, 0x66, 0x66, 0x33, 0x33, 0x30,

    /* U+005D "]" */
    0xff, 0xce, 0x73, 0x9c, 0xe7, 0x39, 0xce, 0x73,
    0xff, 0xe0,

    /* U+005E "^" */
    0x1c, 0x1e, 0xf, 0x8e, 0xc6, 0x36, 0x18,

    /* U+005F "_" */
    0xff,

    /* U+0060 "`" */
    0x71, 0xc0,

    /* U+0061 "a" */
    0x3e, 0x3f, 0xee, 0x38, 0xfe, 0xff, 0xb8, 0xee,
    0x3b, 0xff, 0x79, 0xc0,

    /* U+0062 "b" */
    0xe0, 0x70, 0x38, 0x1f, 0xcf, 0xf7, 0x1f, 0x8f,
    0xc7, 0xe3, 0xf1, 0xff, 0xdf, 0xc0,

    /* U+0063 "c" */
    0x3e, 0x3f, 0xb8, 0xfc, 0x7e, 0x7, 0x1f, 0x8e,
    0xfe, 0x3e, 0x0,

    /* U+0064 "d" */
    0x3, 0x81, 0xc0, 0xe7, 0xf7, 0xff, 0x3f, 0x8f,
    0xc7, 0xe3, 0xf3, 0xdf, 0xe7, 0xf0,

    /* U+0065 "e" */
    0x3e, 0x3f, 0xb8, 0xff, 0xff, 0xff, 0x3, 0x8e,
    0xfe, 0x3e, 0x0,

    /* U+0066 "f" */
    0x3d, 0xf7, 0x3f, 0xfd, 0xc7, 0x1c, 0x71, 0xc7,
    0x1c,

    /* U+0067 "g" */
    0x1, 0xc0, 0x73, 0xf9, 0xfe, 0x73, 0x9f, 0xe3,
    0xf1, 0x80, 0x60, 0x1f, 0xe7, 0xff, 0x87, 0xff,
    0xdf, 0xe0,

    /* U+0068 "h" */
    0xe0, 0x70, 0x38, 0x1d, 0xef, 0xff, 0xff, 0x8f,
    0xc7, 0xe3, 0xf1, 0xf8, 0xfc, 0x70,

    /* U+0069 "i" */
    0xfc, 0x7f, 0xff, 0xff, 0xf0,

    /* U+006A "j" */
    0x39, 0xc0, 0x73, 0x9c, 0xe7, 0x39, 0xce, 0x73,
    0xff, 0xc0,

    /* U+006B "k" */
    0xe0, 0x38, 0xe, 0x3, 0x9e, 0xe7, 0x3b, 0x8f,
    0xc3, 0xf8, 0xfe, 0x3f, 0xce, 0x7b, 0x8e,

    /* U+006C "l" */
    0xff, 0xff, 0xff, 0xff, 0xf0,

    /* U+006D "m" */
    0xef, 0x3d, 0xff, 0xff, 0x8e, 0x3f, 0x1c, 0x7e,
    0x38, 0xfc, 0x71, 0xf8, 0xe3, 0xf1, 0xc7, 0xe3,
    0x8e,

    /* U+006E "n" */
    0xef, 0x7f, 0xf8, 0xfc, 0x7e, 0x3f, 0x1f, 0x8f,
    0xc7, 0xe3, 0x80,

    /* U+006F "o" */
    0x3e, 0x3f, 0xb8, 0xfc, 0x7e, 0x3f, 0x1f, 0x8e,
    0xfe, 0x3e, 0x0,

    /* U+0070 "p" */
    0xfe, 0x7f, 0xb8, 0xfc, 0x7e, 0x3f, 0x1f, 0x8f,
    0xfe, 0xfe, 0x70, 0x38, 0x1c, 0x0,

    /* U+0071 "q" */
    0x3f, 0xbf, 0xf9, 0xfc, 0x7e, 0x3f, 0x1f, 0x9e,
    0xff, 0x3f, 0x81, 0xc0, 0xe0, 0x70,

    /* U+0072 "r" */
    0xff, 0xff, 0xfc, 0xe3, 0x8e, 0x38, 0xe0,

    /* U+0073 "s" */
    0x3e, 0x7f, 0xf8, 0xff, 0x87, 0xf0, 0x7f, 0x8f,
    0xff, 0x3e, 0x0,

    /* U+0074 "t" */
    0x31, 0xcf, 0xff, 0x71, 0xc7, 0x1c, 0x79, 0xf3,
    0xc0,

    /* U+0075 "u" */
    0xe3, 0xf1, 0xf8, 0xfc, 0x7e, 0x3f, 0x1f, 0x8f,
    0xff, 0x7b, 0x80,

    /* U+0076 "v" */
    0xf3, 0xdc, 0xe7, 0x39, 0xce, 0x3f, 0xf, 0xc3,
    0xe0, 0x78, 0x1e, 0x0,

    /* U+0077 "w" */
    0xf3, 0x8e, 0xe7, 0x9d, 0xcf, 0x73, 0xbe, 0xe3,
    0xed, 0xc7, 0xdf, 0xf, 0xbe, 0xf, 0x7c, 0x1c,
    0x70,

    /* U+0078 "x" */
    0x71, 0xcf, 0x70, 0xfe, 0xf, 0x81, 0xf0, 0x7e,
    0xf, 0xe3, 0x9e, 0xf1, 0xc0,

    /* U+0079 "y" */
    0x71, 0xdc, 0x77, 0x38, 0xee, 0x3b, 0x8e, 0xc1,
    0xf0, 0x78, 0xe, 0x7, 0x87, 0xc1, 0xe0,

    /* U+007A "z" */
    0xff, 0xff, 0x1e, 0x1c, 0x3c, 0x78, 0x78, 0xff,
    0xff,

    /* U+007B "{" */
    0x3b, 0xdc, 0xe7, 0x39, 0xdc, 0xe3, 0x9c, 0xe7,
    0x39, 0xe7,

    /* U+007C "|" */
    0xff, 0xff, 0xff, 0xfc,

    /* U+007D "}" */
    0xf3, 0xe3, 0x8e, 0x38, 0xe3, 0x87, 0x1c, 0xe3,
    0x8e, 0x38, 0xef, 0xbc,

    /* U+007E "~" */
    0x78, 0xff, 0xe3, 0xc0,

    /* U+00B0 "°" */
    0x77, 0xe3, 0xf7, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 85, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 85, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 6, .adv_w = 128, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 10, .adv_w = 169, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 24, .adv_w = 171, .box_w = 9, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 40, .adv_w = 256, .box_w = 14, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 60, .adv_w = 228, .box_w = 13, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 78, .adv_w = 71, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 80, .adv_w = 100, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 90, .adv_w = 100, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 100, .adv_w = 142, .box_w = 5, .box_h = 6, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 104, .adv_w = 169, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 113, .adv_w = 85, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 116, .adv_w = 85, .box_w = 4, .box_h = 2, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 117, .adv_w = 85, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 119, .adv_w = 71, .box_w = 4, .box_h = 13, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 126, .adv_w = 171, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 139, .adv_w = 171, .box_w = 8, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 150, .adv_w = 171, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 163, .adv_w = 171, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 176, .adv_w = 171, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 190, .adv_w = 171, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 203, .adv_w = 171, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 216, .adv_w = 171, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 229, .adv_w = 171, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 242, .adv_w = 171, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 255, .adv_w = 85, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 259, .adv_w = 85, .box_w = 3, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 264, .adv_w = 169, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 275, .adv_w = 169, .box_w = 8, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 281, .adv_w = 169, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 292, .adv_w = 156, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 303, .adv_w = 189, .box_w = 11, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 323, .adv_w = 199, .box_w = 12, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 340, .adv_w = 199, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 354, .adv_w = 199, .box_w = 11, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 370, .adv_w = 199, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 384, .adv_w = 185, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 398, .adv_w = 171, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 411, .adv_w = 213, .box_w = 11, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 427, .adv_w = 213, .box_w = 11, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 443, .adv_w = 100, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 448, .adv_w = 171, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 461, .adv_w = 213, .box_w = 11, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 477, .adv_w = 171, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 490, .adv_w = 242, .box_w = 13, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 508, .adv_w = 213, .box_w = 11, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 524, .adv_w = 213, .box_w = 12, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 541, .adv_w = 185, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 554, .adv_w = 213, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 574, .adv_w = 199, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 588, .adv_w = 185, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 602, .adv_w = 185, .box_w = 11, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 618, .adv_w = 213, .box_w = 11, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 634, .adv_w = 199, .box_w = 12, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 651, .adv_w = 256, .box_w = 16, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 673, .adv_w = 199, .box_w = 12, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 690, .adv_w = 199, .box_w = 11, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 706, .adv_w = 185, .box_w = 11, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 722, .adv_w = 100, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 732, .adv_w = 71, .box_w = 4, .box_h = 13, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 739, .adv_w = 100, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 749, .adv_w = 169, .box_w = 9, .box_h = 6, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 756, .adv_w = 128, .box_w = 8, .box_h = 1, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 757, .adv_w = 85, .box_w = 5, .box_h = 2, .ofs_x = 0, .ofs_y = 10},
    {.bitmap_index = 759, .adv_w = 171, .box_w = 10, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 771, .adv_w = 171, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 785, .adv_w = 171, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 796, .adv_w = 171, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 810, .adv_w = 171, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 821, .adv_w = 100, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 830, .adv_w = 171, .box_w = 10, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 848, .adv_w = 171, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 862, .adv_w = 85, .box_w = 3, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 867, .adv_w = 85, .box_w = 5, .box_h = 15, .ofs_x = -1, .ofs_y = -3},
    {.bitmap_index = 877, .adv_w = 171, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 892, .adv_w = 85, .box_w = 3, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 897, .adv_w = 256, .box_w = 15, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 914, .adv_w = 171, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 925, .adv_w = 171, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 936, .adv_w = 171, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 950, .adv_w = 171, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 964, .adv_w = 114, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 971, .adv_w = 156, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 982, .adv_w = 114, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 991, .adv_w = 171, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1002, .adv_w = 156, .box_w = 10, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1014, .adv_w = 242, .box_w = 15, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1031, .adv_w = 171, .box_w = 11, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1044, .adv_w = 156, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1059, .adv_w = 142, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1068, .adv_w = 100, .box_w = 5, .box_h = 16, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 1078, .adv_w = 71, .box_w = 2, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1082, .adv_w = 100, .box_w = 6, .box_h = 16, .ofs_x = 0, .ofs_y = -4},
    {.bitmap_index = 1094, .adv_w = 169, .box_w = 9, .box_h = 3, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 1098, .adv_w = 102, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 7}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 176, .range_length = 1, .glyph_id_start = 96,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Pair left and right glyphs for kerning*/
static const uint8_t kern_pair_glyph_ids[] =
{
    34, 36,
    34, 40,
    34, 48,
    34, 50,
    34, 53,
    34, 54,
    34, 55,
    34, 56,
    34, 58,
    34, 81,
    34, 86,
    34, 87,
    35, 13,
    35, 15,
    35, 34,
    35, 54,
    36, 13,
    36, 15,
    37, 13,
    37, 15,
    37, 34,
    37, 55,
    37, 56,
    37, 58,
    39, 13,
    39, 15,
    39, 34,
    39, 66,
    39, 70,
    39, 77,
    39, 80,
    39, 83,
    40, 13,
    40, 15,
    43, 13,
    43, 15,
    43, 34,
    43, 66,
    43, 70,
    43, 80,
    43, 86,
    43, 90,
    44, 36,
    44, 40,
    44, 48,
    44, 70,
    44, 80,
    44, 86,
    44, 87,
    44, 90,
    45, 36,
    45, 40,
    45, 48,
    45, 53,
    45, 54,
    45, 55,
    45, 56,
    45, 58,
    45, 88,
    45, 90,
    47, 34,
    48, 13,
    48, 15,
    48, 34,
    48, 53,
    48, 55,
    48, 56,
    48, 57,
    48, 58,
    49, 13,
    49, 15,
    49, 34,
    49, 66,
    49, 70,
    49, 80,
    50, 13,
    50, 15,
    50, 34,
    50, 53,
    50, 55,
    50, 58,
    51, 36,
    51, 40,
    51, 48,
    51, 50,
    51, 53,
    51, 54,
    51, 55,
    51, 58,
    51, 70,
    51, 80,
    51, 86,
    51, 88,
    53, 13,
    53, 14,
    53, 15,
    53, 27,
    53, 28,
    53, 34,
    53, 36,
    53, 40,
    53, 48,
    53, 50,
    53, 66,
    53, 68,
    53, 70,
    53, 73,
    53, 77,
    53, 78,
    53, 80,
    53, 83,
    53, 84,
    53, 86,
    53, 88,
    53, 90,
    53, 91,
    54, 13,
    54, 15,
    54, 34,
    55, 13,
    55, 14,
    55, 15,
    55, 27,
    55, 28,
    55, 34,
    55, 36,
    55, 40,
    55, 48,
    55, 50,
    55, 66,
    55, 70,
    55, 74,
    55, 80,
    55, 83,
    55, 86,
    56, 13,
    56, 14,
    56, 15,
    56, 36,
    56, 40,
    56, 48,
    56, 66,
    56, 69,
    56, 70,
    56, 73,
    56, 74,
    56, 80,
    56, 86,
    57, 36,
    57, 40,
    57, 48,
    58, 13,
    58, 14,
    58, 15,
    58, 27,
    58, 28,
    58, 34,
    58, 36,
    58, 40,
    58, 48,
    58, 52,
    58, 66,
    58, 69,
    58, 70,
    58, 80,
    58, 81,
    58, 82,
    58, 86,
    58, 87,
    66, 67,
    66, 72,
    66, 81,
    66, 85,
    66, 87,
    67, 67,
    67, 87,
    68, 73,
    68, 76,
    68, 77,
    68, 90,
    69, 69,
    69, 88,
    70, 67,
    70, 72,
    70, 87,
    70, 89,
    71, 1,
    71, 2,
    71, 13,
    71, 15,
    71, 32,
    71, 66,
    71, 70,
    71, 71,
    71, 76,
    71, 80,
    72, 72,
    72, 77,
    72, 83,
    72, 90,
    76, 70,
    76, 80,
    76, 90,
    77, 90,
    79, 87,
    80, 13,
    80, 15,
    80, 87,
    80, 88,
    80, 89,
    80, 90,
    80, 91,
    81, 13,
    81, 15,
    81, 88,
    81, 90,
    83, 13,
    83, 15,
    83, 27,
    83, 28,
    83, 66,
    83, 68,
    83, 69,
    83, 70,
    83, 72,
    83, 75,
    83, 76,
    83, 77,
    83, 78,
    83, 79,
    83, 80,
    83, 81,
    83, 82,
    83, 83,
    83, 84,
    83, 85,
    83, 86,
    83, 87,
    83, 90,
    84, 88,
    87, 13,
    87, 15,
    87, 68,
    87, 69,
    87, 70,
    87, 80,
    87, 82,
    88, 13,
    88, 15,
    88, 66,
    88, 68,
    88, 69,
    88, 70,
    88, 73,
    88, 80,
    88, 82,
    89, 68,
    89, 69,
    89, 70,
    89, 80,
    90, 13,
    90, 15,
    90, 66,
    90, 68,
    90, 69,
    90, 70,
    90, 80,
    91, 68,
    91, 69,
    91, 70,
    91, 80
};

/* Kerning between the respective left and right glyphs
 * 4.4 format which needs to scaled with `kern_scale`*/
static const int8_t kern_pair_values[] =
{
    -5, -5, -5, -5, -18, -9, -14, 2,
    -22, 4, -3, -4, 6, 4, -4, -7,
    6, 4, -3, -4, -11, -9, 4, -9,
    -37, -39, -24, -7, -9, 2, -9, -9,
    4, 3, -5, -7, -7, -4, -7, -7,
    -7, -4, -9, -9, -9, -9, -9, -7,
    -4, -4, -4, -4, -4, -13, -7, -13,
    -4, -20, -4, -3, -3, -4, -7, -9,
    -7, -11, -4, -13, -17, -46, -48, -22,
    -4, -7, -7, 8, 4, 4, -5, -11,
    -13, -5, -5, -4, -5, -2, -4, -4,
    -11, -6, -7, -4, 4, -37, -17, -39,
    -9, -9, -17, -9, -9, -9, -9, -13,
    -15, -15, 7, 5, -11, -17, -11, -17,
    -11, -3, -4, -9, -7, -9, -9, -31,
    -11, -33, -7, -7, -15, -9, -9, -9,
    -9, -11, -13, 4, -13, -9, -9, -11,
    -3, -13, -4, -4, -4, -4, -7, -7,
    8, 8, -7, -4, -9, -9, -9, -41,
    -22, -44, -15, -15, -24, -15, -15, -15,
    -11, -22, -24, -24, -22, -13, -22, -15,
    -7, 3, 4, 3, -4, -4, 3, -7,
    -4, -4, -7, -4, 3, 3, -3, 6,
    -7, -7, 15, 21, -13, -15, 21, -3,
    -4, 6, 3, -4, 3, 3, 5, 4,
    -9, -9, 4, 4, -7, -3, -4, -9,
    -5, -9, -4, 4, -3, -4, -7, -4,
    -17, -20, 11, 11, 4, 4, 6, 4,
    6, 11, 8, 8, 13, 13, 4, 11,
    6, 11, 6, 13, 8, 15, 15, -7,
    -22, -24, -7, -4, -7, -7, -6, -20,
    -22, -3, -3, -3, -3, 4, -4, -4,
    -4, -4, -4, -4, -22, -24, -4, -4,
    -4, -4, -4, -3, -3, -3, -3
};

/*Collect the kern pair's data in one place*/
static const lv_font_fmt_txt_kern_pair_t kern_pairs =
{
    .glyph_ids = kern_pair_glyph_ids,
    .values = kern_pair_values,
    .pair_cnt = 271,
    .glyph_ids_size = 0
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_pairs,
    .kern_scale = 16,
    .cmap_num = 2,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t ui_font_FontCelestial16 = {
#else
lv_font_t ui_font_FontCelestial16 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 16,          /*The maximum line height required by the font*/
    .base_line = 4,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if UI_FONT_FONTCELESTIAL16*/

