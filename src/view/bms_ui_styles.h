#ifndef BMS_UI_STYLES_H
#define BMS_UI_STYLES_H

#include "lvgl.h"

/* Colors */
#define COLOR_BG          lv_color_make(0x22, 0x22, 0x22)
#define COLOR_GOLD        lv_color_make(0xFF, 0x9F, 0x00)
#define COLOR_CYAN        lv_color_make(0x00, 0xE5, 0xFF)
#define COLOR_RED         lv_color_make(0xFF, 0x17, 0x44)
#define COLOR_GREEN       lv_color_make(0x00, 0xE6, 0x76)
#define COLOR_GRAY        lv_color_make(0x55, 0x66, 0x77)
#define COLOR_DARK_GRAY   lv_color_make(0x11, 0x16, 0x1B)
#define COLOR_HUD_BLUE    lv_color_make(0x00, 0x66, 0xBB)
#define COLOR_DS_TEAL     lv_color_make(0x00, 0xFF, 0xBB)
#define COLOR_DARK_BLUE_1 lv_color_make(0x00, 0x4C, 0x8C)
#define COLOR_DARK_BLUE_2 lv_color_make(0x00, 0x33, 0x5D)
#define COLOR_DARK_BLUE_3 lv_color_make(0x00, 0x1A, 0x2E)

/* Style accessors */
void bms_ui_styles_init(void);
lv_style_t* bms_ui_style_bg(void);
lv_style_t* bms_ui_style_text_gold(void);
lv_style_t* bms_ui_style_text_cyan(void);
lv_style_t* bms_ui_style_text_gray(void);
lv_style_t* bms_ui_style_btn_normal(void);
lv_style_t* bms_ui_style_btn_focused(void);
lv_style_t* bms_ui_style_btn_editing(void);

#endif /* BMS_UI_STYLES_H */
