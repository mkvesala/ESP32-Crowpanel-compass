/*******************************************************************************
 * Size: 14 px
 * Bpp: 1
 * Opts: --bpp 1 --size 14 --font /Users/mattivesala/Documents/Arduino/ESP32-Crowpanel-compass/UI/squareline_project_v3/assets/ArchivoBlack-Regular.ttf -o /Users/mattivesala/Documents/Arduino/ESP32-Crowpanel-compass/UI/squareline_project_v3/assets/ui_font_FontCelestial16.c --format lvgl -r 0x20-0x7f --symbols ° --no-compress --no-prefilter
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
    0xff, 0xff, 0xc0, 0xfc,

    /* U+0022 "\"" */
    0xff, 0xff, 0xd2,

    /* U+0023 "#" */
    0x1b, 0x9, 0x1f, 0xef, 0xf3, 0x61, 0xb3, 0xfd,
    0xfe, 0x24, 0x36, 0x0,

    /* U+0024 "$" */
    0x10, 0x10, 0x3c, 0xff, 0xe7, 0xfc, 0x3f, 0xe7,
    0xff, 0x7e, 0x10, 0x10,

    /* U+0025 "%" */
    0x70, 0xc6, 0xcc, 0x36, 0x41, 0xb6, 0x7, 0x20,
    0x3, 0x78, 0x36, 0x61, 0x33, 0x19, 0x98, 0x87,
    0x80,

    /* U+0026 "&" */
    0x3f, 0xf, 0xfc, 0xe1, 0xce, 0x0, 0x7d, 0xf7,
    0xdf, 0xe1, 0xce, 0x1c, 0xff, 0xc3, 0xf0,

    /* U+0027 "'" */
    0xff, 0xa0,

    /* U+0028 "(" */
    0x19, 0x9c, 0xce, 0x73, 0x9c, 0xe3, 0x1c, 0x61,
    0x80,

    /* U+0029 ")" */
    0xc3, 0x1c, 0x63, 0x9c, 0xe7, 0x39, 0x9c, 0xcc,
    0x0,

    /* U+002A "*" */
    0x27, 0x4d, 0xd2, 0x0,

    /* U+002B "+" */
    0x18, 0x30, 0x67, 0xff, 0xe3, 0x6, 0xc,

    /* U+002C "," */
    0xff, 0xb5, 0x80,

    /* U+002D "-" */
    0xfc,

    /* U+002E "." */
    0xff, 0x80,

    /* U+002F "/" */
    0x33, 0x22, 0x66, 0x64, 0x4c, 0xcc,

    /* U+0030 "0" */
    0x3c, 0x7e, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7,
    0x7e, 0x3c,

    /* U+0031 "1" */
    0x19, 0xf3, 0xe1, 0xc3, 0x87, 0xe, 0x1c, 0xff,
    0xfc,

    /* U+0032 "2" */
    0x3c, 0x7f, 0xe7, 0xe7, 0xf, 0x1e, 0x3c, 0x78,
    0xff, 0xff,

    /* U+0033 "3" */
    0x3e, 0x7f, 0xe7, 0x7, 0xe, 0xe, 0x7, 0xe7,
    0xff, 0x3e,

    /* U+0034 "4" */
    0x1c, 0x1d, 0x8e, 0xce, 0xe6, 0x77, 0x3b, 0xff,
    0xff, 0x7, 0x3, 0x80,

    /* U+0035 "5" */
    0x7e, 0x7e, 0x60, 0x7e, 0x7f, 0x67, 0x7, 0xe7,
    0x7e, 0x3c,

    /* U+0036 "6" */
    0x3e, 0x7f, 0xe7, 0xfe, 0xff, 0xe7, 0xe7, 0xe7,
    0x7e, 0x3c,

    /* U+0037 "7" */
    0xff, 0xff, 0x6, 0xe, 0x1c, 0x1c, 0x18, 0x38,
    0x38, 0x38,

    /* U+0038 "8" */
    0x3c, 0xff, 0xe7, 0xe7, 0x7e, 0x7e, 0xe7, 0xe7,
    0xff, 0x7e,

    /* U+0039 "9" */
    0x3c, 0xfe, 0xe7, 0xe7, 0xff, 0x7f, 0xe7, 0xe7,
    0x7e, 0x3c,

    /* U+003A ":" */
    0xff, 0x81, 0xff,

    /* U+003B ";" */
    0xff, 0x81, 0xff, 0x6b, 0x0,

    /* U+003C "<" */
    0x3, 0xf, 0x7e, 0xf0, 0xe0, 0x7c, 0x1f, 0x3,

    /* U+003D "=" */
    0xff, 0xfc, 0x7, 0xff, 0xe0,

    /* U+003E ">" */
    0x80, 0xf0, 0x7c, 0x1f, 0xf, 0x3e, 0xf0, 0xc0,

    /* U+003F "?" */
    0x3d, 0xff, 0xbf, 0x70, 0xe1, 0x83, 0x0, 0xc,
    0x18,

    /* U+0040 "@" */
    0x1e, 0x8, 0x64, 0xb, 0x79, 0xbf, 0x60, 0xd9,
    0xf6, 0xcd, 0xbf, 0xef, 0x64, 0x1, 0x80, 0x1e,
    0x0,

    /* U+0041 "A" */
    0x1e, 0x3, 0xe0, 0x7c, 0x1d, 0x83, 0xb8, 0x77,
    0x1f, 0xf3, 0xfe, 0x71, 0xdc, 0x3c,

    /* U+0042 "B" */
    0xff, 0x7f, 0xf8, 0xfc, 0x7f, 0xf7, 0xfb, 0x8f,
    0xc7, 0xff, 0xff, 0x80,

    /* U+0043 "C" */
    0x1f, 0x1f, 0xef, 0x3f, 0x87, 0xe0, 0x38, 0xe,
    0x1f, 0xcf, 0x7f, 0x87, 0xc0,

    /* U+0044 "D" */
    0xfe, 0x7f, 0xb9, 0xfc, 0x7e, 0x3f, 0x1f, 0x8f,
    0xcf, 0xff, 0x7f, 0x0,

    /* U+0045 "E" */
    0xff, 0xff, 0xf8, 0x1c, 0xf, 0xf7, 0xfb, 0x81,
    0xc0, 0xff, 0xff, 0xc0,

    /* U+0046 "F" */
    0xff, 0xff, 0xe0, 0xe0, 0xfe, 0xfe, 0xe0, 0xe0,
    0xe0, 0xe0,

    /* U+0047 "G" */
    0x1f, 0x1f, 0xef, 0x1f, 0x80, 0xe7, 0xf9, 0xfe,
    0x1f, 0xcf, 0x7f, 0xcf, 0xb0,

    /* U+0048 "H" */
    0xe1, 0xf8, 0x7e, 0x1f, 0x87, 0xff, 0xff, 0xfe,
    0x1f, 0x87, 0xe1, 0xf8, 0x70,

    /* U+0049 "I" */
    0xff, 0xff, 0xff, 0xfc,

    /* U+004A "J" */
    0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0xe7,
    0xff, 0x7c,

    /* U+004B "K" */
    0xe3, 0xf9, 0xee, 0xf3, 0xb8, 0xfe, 0x3f, 0xcf,
    0xf3, 0x9e, 0xe3, 0xb8, 0xf0,

    /* U+004C "L" */
    0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0,
    0xff, 0xff,

    /* U+004D "M" */
    0xf8, 0xff, 0x9f, 0xf9, 0xff, 0xdf, 0xfd, 0xff,
    0xff, 0xef, 0x7e, 0xf7, 0xef, 0x7e, 0x67,

    /* U+004E "N" */
    0xe1, 0xfc, 0x7f, 0x9f, 0xf7, 0xff, 0xff, 0xfe,
    0xff, 0x9f, 0xe3, 0xf8, 0x70,

    /* U+004F "O" */
    0x3e, 0x1f, 0xef, 0x3f, 0x87, 0xe1, 0xf8, 0x7e,
    0x1f, 0xcf, 0x7f, 0x8f, 0x80,

    /* U+0050 "P" */
    0xfe, 0xff, 0xe7, 0xe7, 0xe7, 0xff, 0xfe, 0xe0,
    0xe0, 0xe0,

    /* U+0051 "Q" */
    0x3e, 0x1f, 0xef, 0x3f, 0x87, 0xe1, 0xf8, 0x7e,
    0x1f, 0xcf, 0x7f, 0x8f, 0xc0, 0x78, 0xe,

    /* U+0052 "R" */
    0xff, 0x7f, 0xf8, 0xfc, 0x7f, 0xff, 0xf3, 0xbd,
    0xce, 0xe7, 0xf1, 0xc0,

    /* U+0053 "S" */
    0x3e, 0x7f, 0xb8, 0xff, 0xf, 0xf3, 0xfc, 0x3f,
    0xc7, 0xff, 0x1f, 0x0,

    /* U+0054 "T" */
    0xff, 0xff, 0xc7, 0x3, 0x81, 0xc0, 0xe0, 0x70,
    0x38, 0x1c, 0xe, 0x0,

    /* U+0055 "U" */
    0xe1, 0xf8, 0x7e, 0x1f, 0x87, 0xe1, 0xf8, 0x7e,
    0x1f, 0xcf, 0x7f, 0x8f, 0xc0,

    /* U+0056 "V" */
    0xf1, 0xce, 0x39, 0xc7, 0x3d, 0xc3, 0xb8, 0x77,
    0xe, 0xc0, 0xf8, 0x1f, 0x3, 0xc0,

    /* U+0057 "W" */
    0xe7, 0x9f, 0xde, 0x77, 0x7b, 0x9d, 0xee, 0x77,
    0xf9, 0xff, 0xe3, 0xcf, 0xf, 0x3c, 0x3c, 0xf0,
    0xf3, 0xc0,

    /* U+0058 "X" */
    0x71, 0xcf, 0x78, 0xfe, 0xf, 0x81, 0xf0, 0x3e,
    0xf, 0xe1, 0xdc, 0x73, 0xde, 0x3c,

    /* U+0059 "Y" */
    0xf1, 0xcf, 0x38, 0xee, 0x1f, 0xc1, 0xf0, 0x1c,
    0x3, 0x80, 0x70, 0xe, 0x1, 0xc0,

    /* U+005A "Z" */
    0xff, 0xff, 0xc3, 0xc3, 0xc1, 0xe1, 0xe0, 0xe0,
    0xf0, 0xff, 0xff, 0xc0,

    /* U+005B "[" */
    0xff, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xff,

    /* U+005C "\\" */
    0xcc, 0xc4, 0x66, 0x66, 0x23, 0x33,

    /* U+005D "]" */
    0xff, 0x33, 0x33, 0x33, 0x33, 0x33, 0xff,

    /* U+005E "^" */
    0x38, 0x71, 0xb3, 0x3c, 0x60,

    /* U+005F "_" */
    0xfe,

    /* U+0060 "`" */
    0x63,

    /* U+0061 "a" */
    0x7e, 0x7f, 0xb9, 0xcf, 0xef, 0xf7, 0x3b, 0xfe,
    0xf7,

    /* U+0062 "b" */
    0xe0, 0xe0, 0xe0, 0xfe, 0xff, 0xe7, 0xe7, 0xe7,
    0xe7, 0xff, 0xfe,

    /* U+0063 "c" */
    0x3c, 0x7f, 0xe7, 0xe0, 0xe0, 0xe7, 0x7f, 0x3c,

    /* U+0064 "d" */
    0x7, 0x7, 0x7, 0x7f, 0xff, 0xe7, 0xe7, 0xe7,
    0xe7, 0xff, 0x7f,

    /* U+0065 "e" */
    0x3e, 0x3f, 0xb8, 0xdf, 0xef, 0xff, 0x19, 0xfc,
    0x7c,

    /* U+0066 "f" */
    0x3b, 0xdd, 0xff, 0xb9, 0xce, 0x73, 0x9c,

    /* U+0067 "g" */
    0x3, 0x81, 0xdf, 0xdf, 0xee, 0x73, 0xf9, 0xf9,
    0xc0, 0xff, 0x3f, 0xf8, 0xff, 0xf7, 0xf0,

    /* U+0068 "h" */
    0xe0, 0xe0, 0xe0, 0xee, 0xff, 0xe7, 0xe7, 0xe7,
    0xe7, 0xe7, 0xe7,

    /* U+0069 "i" */
    0xfc, 0x7f, 0xff, 0xff, 0x80,

    /* U+006A "j" */
    0x39, 0xc0, 0x73, 0x9c, 0xe7, 0x39, 0xce, 0x7f,
    0xf8,

    /* U+006B "k" */
    0xe0, 0x70, 0x38, 0x1c, 0xee, 0xe7, 0xf3, 0xf1,
    0xfc, 0xfe, 0x73, 0xb9, 0xc0,

    /* U+006C "l" */
    0xff, 0xff, 0xff, 0xff, 0x80,

    /* U+006D "m" */
    0xfe, 0x77, 0xff, 0xf9, 0xcf, 0xce, 0x7e, 0x73,
    0xf3, 0x9f, 0x9c, 0xfc, 0xe7,

    /* U+006E "n" */
    0xee, 0xff, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7,

    /* U+006F "o" */
    0x3c, 0x7e, 0xe7, 0xe7, 0xe7, 0xe7, 0x7e, 0x3c,

    /* U+0070 "p" */
    0xfe, 0xff, 0xe7, 0xe7, 0xe7, 0xe7, 0xff, 0xfe,
    0xe0, 0xe0, 0xe0,

    /* U+0071 "q" */
    0x7f, 0xff, 0xe7, 0xe7, 0xe7, 0xe7, 0xff, 0x7f,
    0x7, 0x7, 0x7,

    /* U+0072 "r" */
    0xff, 0xfe, 0x38, 0xe3, 0x8e, 0x38,

    /* U+0073 "s" */
    0x7e, 0xff, 0xe7, 0xfc, 0x3f, 0xe7, 0xff, 0x3e,

    /* U+0074 "t" */
    0x33, 0xbf, 0xf7, 0x39, 0xce, 0x79, 0xc0,

    /* U+0075 "u" */
    0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xff, 0x7f,

    /* U+0076 "v" */
    0xe3, 0x33, 0x9d, 0xce, 0xc3, 0x61, 0xf0, 0xf0,
    0x38,

    /* U+0077 "w" */
    0xe7, 0x3f, 0x3d, 0xdd, 0xec, 0xef, 0x63, 0xdf,
    0x1e, 0xf0, 0xf7, 0x83, 0x9c,

    /* U+0078 "x" */
    0x73, 0xbb, 0x8f, 0x83, 0xc3, 0xe1, 0xf9, 0xdd,
    0xc7,

    /* U+0079 "y" */
    0x73, 0xb9, 0x9c, 0xc6, 0xe3, 0xe1, 0xf0, 0x78,
    0x38, 0x1c, 0x3c, 0x1c, 0x0,

    /* U+007A "z" */
    0xff, 0xfc, 0x71, 0xc7, 0x8e, 0x3f, 0xff,

    /* U+007B "{" */
    0x37, 0x66, 0x66, 0x6c, 0x66, 0x66, 0x77,

    /* U+007C "|" */
    0xff, 0xff, 0xff, 0xf0,

    /* U+007D "}" */
    0xce, 0x66, 0x66, 0x63, 0x66, 0x66, 0xee,

    /* U+007E "~" */
    0x71, 0xff, 0x8e,

    /* U+00B0 "°" */
    0x6f, 0x9f, 0x60
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 75, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 75, .box_w = 3, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 5, .adv_w = 112, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 8, .adv_w = 148, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 20, .adv_w = 149, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 32, .adv_w = 224, .box_w = 13, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 49, .adv_w = 199, .box_w = 12, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 64, .adv_w = 62, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 66, .adv_w = 87, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 75, .adv_w = 87, .box_w = 5, .box_h = 13, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 84, .adv_w = 125, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 88, .adv_w = 148, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 95, .adv_w = 75, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 98, .adv_w = 75, .box_w = 3, .box_h = 2, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 99, .adv_w = 75, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 101, .adv_w = 62, .box_w = 4, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 107, .adv_w = 149, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 117, .adv_w = 149, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 126, .adv_w = 149, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 136, .adv_w = 149, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 146, .adv_w = 149, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 158, .adv_w = 149, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 168, .adv_w = 149, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 178, .adv_w = 149, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 188, .adv_w = 149, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 198, .adv_w = 149, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 208, .adv_w = 75, .box_w = 3, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 211, .adv_w = 75, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 216, .adv_w = 148, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 224, .adv_w = 148, .box_w = 7, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 229, .adv_w = 148, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 237, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 246, .adv_w = 166, .box_w = 10, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 263, .adv_w = 174, .box_w = 11, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 277, .adv_w = 174, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 289, .adv_w = 174, .box_w = 10, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 302, .adv_w = 174, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 314, .adv_w = 162, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 326, .adv_w = 149, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 336, .adv_w = 187, .box_w = 10, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 349, .adv_w = 187, .box_w = 10, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 362, .adv_w = 87, .box_w = 3, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 366, .adv_w = 149, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 376, .adv_w = 187, .box_w = 10, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 389, .adv_w = 149, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 399, .adv_w = 211, .box_w = 12, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 414, .adv_w = 187, .box_w = 10, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 427, .adv_w = 187, .box_w = 10, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 440, .adv_w = 162, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 450, .adv_w = 187, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 465, .adv_w = 174, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 477, .adv_w = 162, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 489, .adv_w = 162, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 501, .adv_w = 187, .box_w = 10, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 514, .adv_w = 174, .box_w = 11, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 528, .adv_w = 224, .box_w = 14, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 546, .adv_w = 174, .box_w = 11, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 560, .adv_w = 174, .box_w = 11, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 574, .adv_w = 162, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 586, .adv_w = 87, .box_w = 4, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 593, .adv_w = 62, .box_w = 4, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 599, .adv_w = 87, .box_w = 4, .box_h = 14, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 606, .adv_w = 148, .box_w = 7, .box_h = 5, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 611, .adv_w = 112, .box_w = 7, .box_h = 1, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 612, .adv_w = 75, .box_w = 4, .box_h = 2, .ofs_x = 0, .ofs_y = 9},
    {.bitmap_index = 613, .adv_w = 149, .box_w = 9, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 622, .adv_w = 149, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 633, .adv_w = 149, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 641, .adv_w = 149, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 652, .adv_w = 149, .box_w = 9, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 661, .adv_w = 87, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 668, .adv_w = 149, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 683, .adv_w = 149, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 694, .adv_w = 75, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 699, .adv_w = 75, .box_w = 5, .box_h = 14, .ofs_x = -1, .ofs_y = -3},
    {.bitmap_index = 708, .adv_w = 149, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 721, .adv_w = 75, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 726, .adv_w = 224, .box_w = 13, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 739, .adv_w = 149, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 747, .adv_w = 149, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 755, .adv_w = 149, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 766, .adv_w = 149, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 777, .adv_w = 99, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 783, .adv_w = 137, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 791, .adv_w = 99, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 798, .adv_w = 149, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 806, .adv_w = 137, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 815, .adv_w = 211, .box_w = 13, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 828, .adv_w = 149, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 837, .adv_w = 137, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 850, .adv_w = 125, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 857, .adv_w = 87, .box_w = 4, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 864, .adv_w = 62, .box_w = 2, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 868, .adv_w = 87, .box_w = 4, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 875, .adv_w = 148, .box_w = 8, .box_h = 3, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 878, .adv_w = 90, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 6}
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
    -4, -4, -4, -4, -15, -8, -12, 2,
    -19, 4, -2, -4, 6, 4, -4, -6,
    6, 4, -2, -4, -10, -8, 4, -8,
    -33, -34, -21, -6, -8, 2, -8, -8,
    4, 2, -4, -6, -6, -4, -6, -6,
    -6, -4, -8, -8, -8, -8, -8, -6,
    -4, -4, -4, -4, -4, -11, -6, -11,
    -4, -17, -4, -2, -2, -4, -6, -8,
    -6, -10, -4, -12, -15, -40, -42, -19,
    -4, -6, -6, 7, 4, 4, -4, -10,
    -11, -4, -4, -4, -4, -2, -4, -4,
    -10, -6, -6, -4, 4, -33, -15, -34,
    -8, -8, -15, -8, -8, -8, -8, -11,
    -13, -13, 6, 4, -10, -15, -10, -15,
    -10, -2, -4, -8, -6, -8, -8, -27,
    -10, -29, -6, -6, -13, -8, -8, -8,
    -8, -10, -11, 4, -11, -8, -8, -10,
    -2, -12, -4, -4, -4, -4, -6, -6,
    7, 7, -6, -4, -8, -8, -8, -36,
    -19, -38, -13, -13, -21, -13, -13, -13,
    -10, -19, -21, -21, -19, -11, -19, -13,
    -6, 2, 4, 2, -4, -4, 2, -6,
    -4, -4, -6, -4, 2, 2, -2, 6,
    -6, -6, 13, 19, -12, -13, 19, -2,
    -4, 6, 2, -4, 2, 2, 4, 4,
    -8, -8, 4, 4, -6, -2, -4, -8,
    -4, -8, -4, 4, -2, -4, -6, -4,
    -15, -17, 9, 9, 4, 4, 6, 4,
    6, 9, 7, 7, 11, 11, 4, 9,
    6, 9, 6, 11, 7, 13, 13, -6,
    -19, -21, -6, -4, -6, -6, -6, -17,
    -19, -2, -2, -2, -2, 4, -4, -4,
    -4, -4, -4, -4, -19, -21, -4, -4,
    -4, -4, -4, -2, -2, -2, -2
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
    .line_height = 14,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
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

