#include "bms_ui_styles.h"

static lv_style_t s_bg, s_textGold, s_textCyan, s_textGray;
static lv_style_t s_btnNormal, s_btnFocused, s_btnEditing;

lv_style_t* bms_ui_style_bg(void)          { return &s_bg; }
lv_style_t* bms_ui_style_text_gold(void)    { return &s_textGold; }
lv_style_t* bms_ui_style_text_cyan(void)    { return &s_textCyan; }
lv_style_t* bms_ui_style_text_gray(void)    { return &s_textGray; }
lv_style_t* bms_ui_style_btn_normal(void)   { return &s_btnNormal; }
lv_style_t* bms_ui_style_btn_focused(void)  { return &s_btnFocused; }
lv_style_t* bms_ui_style_btn_editing(void)  { return &s_btnEditing; }

void bms_ui_styles_init(void)
{
    lv_style_init(&s_bg);
    lv_style_set_bg_color(&s_bg, COLOR_BG);
    lv_style_set_bg_opa(&s_bg, LV_OPA_COVER);
    lv_style_set_pad_all(&s_bg, 0);
    lv_style_set_border_width(&s_bg, 0);
    lv_style_set_radius(&s_bg, 0);

    lv_style_init(&s_textGold);
    lv_style_set_text_color(&s_textGold, COLOR_GOLD);
    lv_style_set_text_font(&s_textGold, &lv_font_montserrat_12);

    lv_style_init(&s_textCyan);
    lv_style_set_text_color(&s_textCyan, COLOR_CYAN);
    lv_style_set_text_font(&s_textCyan, &lv_font_montserrat_12);

    lv_style_init(&s_textGray);
    lv_style_set_text_color(&s_textGray, COLOR_GRAY);
    lv_style_set_text_font(&s_textGray, &lv_font_montserrat_12);

    lv_style_init(&s_btnNormal);
    lv_style_set_bg_color(&s_btnNormal, COLOR_BG);
    lv_style_set_bg_opa(&s_btnNormal, LV_OPA_COVER);
    lv_style_set_border_width(&s_btnNormal, 1);
    lv_style_set_border_color(&s_btnNormal, COLOR_GRAY);
    lv_style_set_text_color(&s_btnNormal, lv_color_white());
    lv_style_set_text_font(&s_btnNormal, &lv_font_montserrat_12);
    lv_style_set_pad_all(&s_btnNormal, 0);
    lv_style_set_radius(&s_btnNormal, 0);

    lv_style_init(&s_btnFocused);
    lv_style_set_border_width(&s_btnFocused, 1);
    lv_style_set_border_color(&s_btnFocused, COLOR_GRAY);
    lv_style_set_outline_width(&s_btnFocused, 0);
    lv_style_set_bg_color(&s_btnFocused, COLOR_BG);
    lv_style_set_bg_opa(&s_btnFocused, LV_OPA_COVER);
    lv_style_set_radius(&s_btnFocused, 0);

    lv_style_init(&s_btnEditing);
    lv_style_set_bg_color(&s_btnEditing, COLOR_GOLD);
    lv_style_set_bg_opa(&s_btnEditing, LV_OPA_COVER);
    lv_style_set_border_width(&s_btnEditing, 1);
    lv_style_set_border_color(&s_btnEditing, COLOR_GOLD);
    lv_style_set_text_color(&s_btnEditing, COLOR_BG);
    lv_style_set_outline_width(&s_btnEditing, 0);
    lv_style_set_radius(&s_btnEditing, 0);
}
