#include "bms_ui_pages.h"
#include "bms_ui_internal.h"
#include "bms_ui_styles.h"
#include <stdio.h>

/**********************
 *  STATIC HELPERS
 **********************/

static void make_seg(lv_obj_t* bar, int w, int x, lv_color_t c)
{
    if(w <= 0) return;
    lv_obj_t* s = lv_obj_create(bar);
    lv_obj_set_size(s, w, 22);
    lv_obj_set_pos(s, x, 0);
    bms_ui_strip_decorations(s);
    lv_obj_set_style_bg_color(s, c, 0);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
}

static lv_obj_t* create_selection_bar(lv_obj_t* parent, int width)
{
    lv_obj_t* bar = lv_obj_create(parent);
    lv_obj_set_size(bar, width, 22);
    lv_obj_set_pos(bar, 0, 0);
    bms_ui_strip_decorations(bar);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
    lv_obj_add_flag(bar, LV_OBJ_FLAG_HIDDEN);

    int w_main = 68, w_d1 = 9, w_d2 = 6, w_d3 = 2;
    if(width != 85) {
        w_main = (width * 80) / 100;
        w_d1 = (width * 10) / 100;
        w_d2 = (width * 20) / 300;
        w_d3 = width - w_main - w_d1 - w_d2;
    }

    make_seg(bar, w_main, 0, COLOR_HUD_BLUE);
    make_seg(bar, w_d1, w_main, COLOR_DARK_BLUE_1);
    make_seg(bar, w_d2, w_main + w_d1, COLOR_DARK_BLUE_2);
    make_seg(bar, w_d3, w_main + w_d1 + w_d2, COLOR_DARK_BLUE_3);
    return bar;
}

static void setup_hud_button(lv_obj_t* btn)
{
    lv_obj_set_size(btn, 85, 22);
    lv_obj_add_style(btn, bms_ui_style_btn_normal(), 0);
    lv_obj_add_style(btn, bms_ui_style_btn_focused(), LV_STATE_FOCUSED);
    lv_obj_add_style(btn, bms_ui_style_btn_focused(), LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn, bms_ui_style_btn_focused(), LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn, bms_ui_style_btn_editing(), LV_STATE_USER_1);
    lv_obj_add_style(btn, bms_ui_style_btn_editing(), LV_STATE_USER_1 | LV_STATE_FOCUS_KEY);

    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 1, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(btn, 1, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(btn, 1, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(btn, COLOR_GRAY, LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(btn, COLOR_GRAY, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(btn, COLOR_GRAY, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);

    create_selection_bar(btn, 85);
}

static lv_obj_t* create_hud_button(lv_obj_t* parent, int x, int y, const char* text)
{
    lv_obj_t* btn = lv_button_create(parent);
    lv_obj_set_pos(btn, x, y);
    setup_hud_button(btn);
    lv_obj_t* lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_center(lbl);
    return btn;
}

static lv_obj_t* create_chart(lv_obj_t* parent, lv_chart_series_t** ser_u, lv_chart_series_t** ser_i)
{
    lv_obj_t* chart = lv_chart_create(parent);
    lv_obj_set_size(chart, 136, 56);
    lv_obj_set_pos(chart, 98, 8);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, 20);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 5000);
    lv_obj_set_style_bg_color(chart, COLOR_BG, 0);
    lv_obj_set_style_border_color(chart, COLOR_DARK_GRAY, 0);
    lv_obj_set_style_border_width(chart, 1, 0);
    lv_obj_set_style_pad_all(chart, 0, 0);
    lv_obj_set_style_line_color(chart, COLOR_DARK_GRAY, LV_PART_ITEMS);
    lv_obj_set_style_width(chart, 0, LV_PART_INDICATOR);
    lv_obj_set_style_height(chart, 0, LV_PART_INDICATOR);

    *ser_u = lv_chart_add_series(chart, COLOR_CYAN, LV_CHART_AXIS_PRIMARY_Y);
    *ser_i = lv_chart_add_series(chart, COLOR_GOLD, LV_CHART_AXIS_PRIMARY_Y);

    for(int i = 0; i < 20; i++) {
        lv_chart_set_next_value(chart, *ser_u, 0);
        lv_chart_set_next_value(chart, *ser_i, 0);
    }
    return chart;
}

static lv_obj_t* create_readout_label(lv_obj_t* parent)
{
    lv_obj_t* lbl = lv_label_create(parent);
    lv_obj_add_style(lbl, bms_ui_style_text_gray(), 0);
    lv_obj_set_style_text_font(lbl, &montserrat_12_subset, 0);
    lv_label_set_text(lbl, "U: 0.00V  I: 0.00A");
    lv_obj_set_pos(lbl, 98, 68);
    return lbl;
}

/**********************
 *  PAGE CREATION
 **********************/

static void create_page_soc(bms_ui_widgets_t* w)
{
    lv_obj_t* p = w->pages[0];

    w->lblSocBig = lv_label_create(p);
    lv_obj_set_style_text_font(w->lblSocBig, &montserrat_28_subset, 0);
    lv_obj_set_style_text_color(w->lblSocBig, COLOR_GOLD, 0);
    lv_label_set_text(w->lblSocBig, "85%");
    lv_obj_set_pos(w->lblSocBig, 10, 10);

    w->lblTrend = lv_label_create(p);
    lv_obj_add_style(w->lblTrend, bms_ui_style_text_gray(), 0);
    lv_label_set_text(w->lblTrend, "[STANDBY]");
    lv_obj_set_pos(w->lblTrend, 10, 44);

    lv_obj_t* desc = lv_label_create(p);
    lv_obj_add_style(desc, bms_ui_style_text_gray(), 0);
    lv_obj_set_style_text_font(desc, &montserrat_12_subset, 0);
    lv_label_set_text(desc, "CHIRAL EST. SOC");
    lv_obj_set_pos(desc, 10, 66);

    w->lblU = lv_label_create(p);
    lv_obj_add_style(w->lblU, bms_ui_style_text_cyan(), 0);
    lv_label_set_text(w->lblU, "U: 4.120 V");
    lv_obj_set_pos(w->lblU, 125, 12);

    w->lblI = lv_label_create(p);
    lv_obj_add_style(w->lblI, bms_ui_style_text_cyan(), 0);
    lv_label_set_text(w->lblI, "I:  0.00 A");
    lv_obj_set_pos(w->lblI, 125, 32);

    w->lblT = lv_label_create(p);
    lv_obj_add_style(w->lblT, bms_ui_style_text_cyan(), 0);
    lv_label_set_text(w->lblT, "T: 25.0 C");
    lv_obj_set_pos(w->lblT, 125, 52);

    w->lblR = lv_label_create(p);
    lv_obj_add_style(w->lblR, bms_ui_style_text_gray(), 0);
    lv_label_set_text(w->lblR, "R: 0.05 Ohm");
    lv_obj_set_pos(w->lblR, 125, 72);
}

static void create_page_cccv(bms_ui_widgets_t* w)
{
    lv_obj_t* p = w->pages[1];

    w->btnUset = create_hud_button(p, 4, 8, "U: 4.20V");
    w->btnIset = create_hud_button(p, 4, 36, "I: 2.00A");

    w->btnChgToggle = lv_button_create(p);
    lv_obj_set_pos(w->btnChgToggle, 4, 64);
    setup_hud_button(w->btnChgToggle);
    bms_ui_view_update_toggle_style(w->btnChgToggle, false);
    lv_obj_t* lbl = lv_label_create(w->btnChgToggle);
    lv_label_set_text(lbl, "CHG: OFF");
    lv_obj_center(lbl);

    w->chartP2 = create_chart(p, &w->p2SerU, &w->p2SerI);
    w->lblP2Readout = create_readout_label(p);
}

static void create_page_discharge(bms_ui_widgets_t* w)
{
    lv_obj_t* p = w->pages[2];

    w->btnIdis = create_hud_button(p, 4, 20, "I: 3.00A");

    w->btnDscToggle = lv_button_create(p);
    lv_obj_set_pos(w->btnDscToggle, 4, 52);
    setup_hud_button(w->btnDscToggle);
    bms_ui_view_update_toggle_style(w->btnDscToggle, false);
    lv_obj_t* lbl = lv_label_create(w->btnDscToggle);
    lv_label_set_text(lbl, "DSC: OFF");
    lv_obj_center(lbl);

    w->chartP3 = create_chart(p, &w->p3SerU, &w->p3SerI);
    w->lblP3Readout = create_readout_label(p);
}

static void create_page_system(bms_ui_widgets_t* w)
{
    lv_obj_t* p = w->pages[3];

    w->btnBaud = create_hud_button(p, 4, 20, "Baud:115K");
    w->btnPort = create_hud_button(p, 4, 52, "Port:UART0");

    w->lblTerminal = lv_label_create(p);
    lv_obj_add_style(w->lblTerminal, bms_ui_style_text_gold(), 0);
    lv_obj_set_style_text_font(w->lblTerminal, &montserrat_12_subset, 0);
    lv_obj_set_pos(w->lblTerminal, 98, 8);
    lv_obj_set_size(w->lblTerminal, 136, 80);
    lv_label_set_long_mode(w->lblTerminal, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_radius(w->lblTerminal, 4, 0);
    lv_obj_set_style_bg_color(w->lblTerminal, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(w->lblTerminal, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(w->lblTerminal, 1, 0);
    lv_obj_set_style_border_color(w->lblTerminal, lv_color_black(), 0);
    lv_obj_set_style_pad_all(w->lblTerminal, 3, 0);
}

/**********************
 *  HEADER / FOOTER
 **********************/

static void create_header_footer(bms_ui_widgets_t* w)
{
    lv_obj_t* scr = lv_screen_active();

    lv_obj_t* header = lv_obj_create(scr);
    lv_obj_set_size(header, 240, 18);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_add_style(header, bms_ui_style_bg(), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_scrollbar_mode(header, LV_SCROLLBAR_MODE_OFF);

    w->lblMode = lv_label_create(header);
    lv_obj_add_style(w->lblMode, bms_ui_style_text_cyan(), 0);
    lv_label_set_text(w->lblMode, "[ SoC LNK ]");
    lv_obj_set_pos(w->lblMode, 8, 3);

    w->barSoc = lv_bar_create(header);
    lv_obj_set_size(w->barSoc, 60, 14);
    lv_obj_set_pos(w->barSoc, 115, 2);
    lv_obj_set_style_bg_color(w->barSoc, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(w->barSoc, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(w->barSoc, 1, 0);
    lv_obj_set_style_border_color(w->barSoc, COLOR_CYAN, 0);
    lv_obj_set_style_pad_all(w->barSoc, 0, 0);
    lv_obj_set_style_bg_color(w->barSoc, COLOR_CYAN, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(w->barSoc, LV_OPA_50, LV_PART_INDICATOR);
    lv_obj_set_style_radius(w->barSoc, 0, 0);
    lv_obj_set_style_radius(w->barSoc, 0, LV_PART_INDICATOR);
    lv_bar_set_value(w->barSoc, 85, LV_ANIM_OFF);

    w->lblSoc = lv_label_create(w->barSoc);
    lv_obj_add_style(w->lblSoc, bms_ui_style_text_cyan(), 0);
    lv_obj_set_style_text_color(w->lblSoc, lv_color_white(), 0);
    lv_label_set_text(w->lblSoc, "85%");
    lv_obj_center(w->lblSoc);

    w->lblLink = lv_label_create(header);
    lv_obj_add_style(w->lblLink, bms_ui_style_text_cyan(), 0);
    lv_obj_set_pos(w->lblLink, 180, 3);

    lv_obj_t* line = lv_line_create(scr);
    static lv_point_precise_t line_pts[] = {{0, 18}, {240, 18}};
    lv_line_set_points(line, line_pts, 2);
    lv_obj_set_style_line_width(line, 1, 0);
    lv_obj_set_style_line_color(line, COLOR_GRAY, 0);

    lv_obj_t* footer = lv_obj_create(scr);
    lv_obj_set_size(footer, 240, 15);
    lv_obj_set_pos(footer, 0, 120);
    lv_obj_add_style(footer, bms_ui_style_bg(), 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, 0, 0);
    lv_obj_set_scrollbar_mode(footer, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* line_f = lv_line_create(scr);
    static lv_point_precise_t line_f_pts[] = {{0, 120}, {240, 120}};
    lv_line_set_points(line_f, line_f_pts, 2);
    lv_obj_set_style_line_width(line_f, 1, 0);
    lv_obj_set_style_line_color(line_f, COLOR_GRAY, 0);

    w->circle = lv_obj_create(footer);
    lv_obj_set_size(w->circle, 8, 8);
    lv_obj_set_pos(w->circle, 8, 3);
    lv_obj_set_style_radius(w->circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(w->circle, 0, 0);
    lv_obj_set_style_pad_all(w->circle, 0, 0);
    lv_obj_set_style_shadow_width(w->circle, 0, 0);
    lv_obj_set_style_outline_width(w->circle, 0, 0);
    lv_obj_set_style_bg_color(w->circle, COLOR_GOLD, 0);
    lv_obj_set_style_bg_opa(w->circle, LV_OPA_COVER, 0);

    w->btnFooter = lv_button_create(footer);
    lv_obj_set_size(w->btnFooter, 70, 13);
    lv_obj_set_pos(w->btnFooter, 165, 1);
    lv_obj_set_style_radius(w->btnFooter, 0, 0);
    lv_obj_set_style_border_width(w->btnFooter, 0, 0);
    lv_obj_set_style_shadow_width(w->btnFooter, 0, 0);
    lv_obj_set_style_outline_width(w->btnFooter, 0, 0);
    lv_obj_set_style_pad_all(w->btnFooter, 0, 0);
    lv_obj_set_style_bg_opa(w->btnFooter, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(w->btnFooter, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(w->btnFooter, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(w->btnFooter, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(w->btnFooter, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(w->btnFooter, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(w->btnFooter, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_add_style(w->btnFooter, bms_ui_style_btn_editing(), LV_STATE_USER_1);
    lv_obj_add_style(w->btnFooter, bms_ui_style_btn_editing(), LV_STATE_USER_1 | LV_STATE_FOCUS_KEY);

    w->lblFooter = lv_label_create(w->btnFooter);
    lv_obj_add_style(w->lblFooter, bms_ui_style_text_cyan(), 0);
    lv_label_set_text(w->lblFooter, "[ * - - - ]");
    lv_obj_center(w->lblFooter);
}

/**********************
 *  PUBLIC ENTRY
 **********************/

void bms_ui_pages_create(bms_ui_widgets_t* w)
{
    lv_obj_t* scr = lv_screen_active();

    create_header_footer(w);

    for(int i = 0; i < 4; i++) {
        w->pages[i] = lv_obj_create(scr);
        lv_obj_set_size(w->pages[i], 240, 102);
        lv_obj_set_pos(w->pages[i], 0, 18);
        lv_obj_add_style(w->pages[i], bms_ui_style_bg(), 0);
        lv_obj_add_flag(w->pages[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(w->pages[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(w->pages[i], LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_set_style_border_width(w->pages[i], 0, 0);
        lv_obj_set_style_pad_all(w->pages[i], 0, 0);
        lv_obj_set_scrollbar_mode(w->pages[i], LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_outline_width(w->pages[i], 0, 0);
        lv_obj_set_style_outline_width(w->pages[i], 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(w->pages[i], 0, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(w->pages[i], 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    }

    create_page_soc(w);
    create_page_cccv(w);
    create_page_discharge(w);
    create_page_system(w);
}
