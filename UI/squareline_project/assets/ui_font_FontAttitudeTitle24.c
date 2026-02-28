/*******************************************************************************
 * Size: 24 px
 * Bpp: 1
 * Opts: --bpp 1 --size 24 --font /Users/matti.vesalasofigate.com/Documents/Arduino/ESP32-Crowpanel-compass/UI/squareline_project/assets/ArchivoBlack-Regular.ttf -o /Users/matti.vesalasofigate.com/Documents/Arduino/ESP32-Crowpanel-compass/UI/squareline_project/assets/ui_font_FontAttitudeTitle24.c --format lvgl --symbols PitchRol: --no-compress --no-prefilter
 ******************************************************************************/

#include "ui.h"

#ifndef UI_FONT_FONTATTITUDETITLE24
#define UI_FONT_FONTATTITUDETITLE24 1
#endif

#if UI_FONT_FONTATTITUDETITLE24

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+003A ":" */
    0xff, 0xff, 0xff, 0x80, 0x0, 0xff, 0xff, 0xff,
    0x80,

    /* U+0050 "P" */
    0xff, 0xe3, 0xff, 0xcf, 0xff, 0xbf, 0xff, 0xf8,
    0x7f, 0xe1, 0xff, 0x87, 0xfe, 0x1f, 0xff, 0xff,
    0xff, 0xef, 0xff, 0x3f, 0xf8, 0xf8, 0x3, 0xe0,
    0xf, 0x80, 0x3e, 0x0, 0xf8, 0x0,

    /* U+0052 "R" */
    0xff, 0xf1, 0xff, 0xfb, 0xff, 0xf7, 0xff, 0xff,
    0x87, 0xff, 0x7, 0xfe, 0xf, 0xfc, 0x3f, 0xff,
    0xfd, 0xff, 0xf3, 0xff, 0xc7, 0xcf, 0xcf, 0x8f,
    0x9f, 0x1f, 0xbe, 0x1f, 0x7c, 0x3f, 0xf8, 0x3e,

    /* U+0063 "c" */
    0xf, 0xc0, 0xff, 0xc7, 0xff, 0x9f, 0x3f, 0xf8,
    0x7f, 0xe0, 0xf, 0x80, 0x3e, 0x0, 0xf8, 0x7d,
    0xf3, 0xf7, 0xff, 0x8f, 0xfc, 0xf, 0xe0,

    /* U+0068 "h" */
    0xf8, 0x7, 0xc0, 0x3e, 0x1, 0xf0, 0xf, 0x9e,
    0x7f, 0xfb, 0xff, 0xff, 0xff, 0xf8, 0xff, 0xc7,
    0xfe, 0x3f, 0xf1, 0xff, 0x8f, 0xfc, 0x7f, 0xe3,
    0xff, 0x1f, 0xf8, 0xf8,

    /* U+0069 "i" */
    0xff, 0xfe, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xf8,

    /* U+006C "l" */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xf8,

    /* U+006F "o" */
    0xf, 0xc0, 0xff, 0xc7, 0xff, 0x9f, 0x3e, 0xf8,
    0x7f, 0xe1, 0xff, 0x87, 0xfe, 0x1f, 0xf8, 0x7d,
    0xf3, 0xe7, 0xff, 0x8f, 0xfc, 0xf, 0xc0,

    /* U+0074 "t" */
    0x1e, 0xf, 0x7, 0x87, 0xcf, 0xff, 0xff, 0xfe,
    0x7c, 0x3e, 0x1f, 0xf, 0x87, 0xc3, 0xe1, 0xf8,
    0xfe, 0x7f, 0xf, 0x80
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 128, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 9, .adv_w = 277, .box_w = 14, .box_h = 17, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 39, .adv_w = 299, .box_w = 15, .box_h = 17, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 71, .adv_w = 256, .box_w = 14, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 94, .adv_w = 256, .box_w = 13, .box_h = 17, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 122, .adv_w = 128, .box_w = 5, .box_h = 17, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 133, .adv_w = 128, .box_w = 5, .box_h = 17, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 144, .adv_w = 256, .box_w = 14, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 167, .adv_w = 170, .box_w = 9, .box_h = 17, .ofs_x = 1, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0x16, 0x18, 0x29, 0x2e, 0x2f, 0x32, 0x35,
    0x3a
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 58, .range_length = 59, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 9, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Pair left and right glyphs for kerning*/
static const uint8_t kern_pair_glyph_ids[] =
{
    2, 8,
    3, 8,
    4, 5,
    4, 7
};

/* Kerning between the respective left and right glyphs
 * 4.4 format which needs to scaled with `kern_scale`*/
static const int8_t kern_pair_values[] =
{
    -10, -10, -7, -10
};

/*Collect the kern pair's data in one place*/
static const lv_font_fmt_txt_kern_pair_t kern_pairs =
{
    .glyph_ids = kern_pair_glyph_ids,
    .values = kern_pair_values,
    .pair_cnt = 4,
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
    .cmap_num = 1,
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
const lv_font_t ui_font_FontAttitudeTitle24 = {
#else
lv_font_t ui_font_FontAttitudeTitle24 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 17,          /*The maximum line height required by the font*/
    .base_line = 0,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if UI_FONT_FONTATTITUDETITLE24*/

