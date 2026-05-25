#include "bms_ui_view.h"
#include <stdio.h>
#include <string.h>

/*********************
 *      DEFINES
 *********************/
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

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_style_t s_styleBg, s_styleTextGold, s_styleTextCyan, s_styleTextGray;
static lv_style_t s_styleBtnNormal, s_styleBtnFocused, s_styleBtnEditing;

static lv_obj_t* s_pages[4];
static int s_currentPage = 0;
static bool s_switchingPage = false;

/* Header/Footer */
static lv_obj_t* s_lblMode;
static lv_obj_t* s_lblSoc;
static lv_obj_t* s_barSoc;
static lv_obj_t* s_lblLink;
static lv_obj_t* s_lblFooter;
static lv_obj_t* s_btnFooter;
static lv_obj_t* s_circle;
static bool s_footerSelected = false;

/* Page 0 */
static lv_obj_t* s_lblSocBig;
static lv_obj_t* s_lblTrend;
static lv_obj_t* s_lblU, *s_lblI, *s_lblT, *s_lblR;

/* Page 1 */
static lv_obj_t* s_btnUset, *s_btnIset, *s_btnChgToggle;
static lv_obj_t* s_chartP2;
static lv_chart_series_t *s_p2SerU, *s_p2SerI;
static lv_obj_t* s_lblP2Readout;

/* Page 2 */
static lv_obj_t* s_btnIdis, *s_btnDscToggle;
static lv_obj_t* s_chartP3;
static lv_chart_series_t *s_p3SerU, *s_p3SerI;
static lv_obj_t* s_lblP3Readout;

/* Page 3 */
static lv_obj_t* s_btnBaud, *s_btnPort;
static lv_obj_t* s_lblTerminal;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void init_styles(void);
static void create_header_footer(void);
static void create_page_soc(void);
static void create_page_cccv(void);
static void create_page_discharge(void);
static void create_page_system(void);
static void setup_hud_button(lv_obj_t* btn);
static lv_obj_t* create_selection_bar(lv_obj_t* parent, int w);
static void strip_decorations(lv_obj_t* obj);
static void update_link_status(bool online);

/**********************
 *   UTILITY
 **********************/

lv_obj_t* bms_ui_view_get_button_label(lv_obj_t* btn)
{
    if(!btn) return NULL;
    uint32_t cnt = lv_obj_get_child_cnt(btn);
    for(uint32_t i = 0; i < cnt; i++) {
        lv_obj_t* child = lv_obj_get_child(btn, i);
        if(child && lv_obj_check_type(child, &lv_label_class)) return child;
    }
    return NULL;
}

lv_obj_t* bms_ui_view_get_button_bar(lv_obj_t* btn)
{
    if(!btn) return NULL;
    uint32_t cnt = lv_obj_get_child_cnt(btn);
    for(uint32_t i = 0; i < cnt; i++) {
        lv_obj_t* child = lv_obj_get_child(btn, i);
        if(child && !lv_obj_check_type(child, &lv_label_class)) return child;
    }
    return NULL;
}

static void strip_decorations(lv_obj_t* obj)
{
    if(!obj) return;
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

/**********************
 *   GETTERS
 **********************/

int  bms_ui_view_current_page(void)      { return s_currentPage; }
bool bms_ui_view_footer_selected(void)    { return s_footerSelected; }
void bms_ui_view_set_footer_selected(bool v) { s_footerSelected = v; }
bool bms_ui_view_switching_page(void)     { return s_switchingPage; }

lv_obj_t* bms_ui_view_btn_charge_toggle(void)    { return s_btnChgToggle; }
lv_obj_t* bms_ui_view_btn_discharge_toggle(void)  { return s_btnDscToggle; }
lv_obj_t* bms_ui_view_footer_menu(void)           { return s_btnFooter; }
lv_obj_t* bms_ui_view_btn_uset(void)              { return s_btnUset; }
lv_obj_t* bms_ui_view_btn_iset(void)              { return s_btnIset; }
lv_obj_t* bms_ui_view_btn_idis(void)              { return s_btnIdis; }
lv_obj_t* bms_ui_view_btn_baud(void)              { return s_btnBaud; }
lv_obj_t* bms_ui_view_btn_port(void)              { return s_btnPort; }

/**********************
 *   INIT
 **********************/

void bms_ui_view_init(void)
{
    lv_group_t* g = lv_group_get_default();
    if(g) lv_group_remove_all_objs(g);
    else { g = lv_group_create(); lv_group_set_default(g); }

    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    init_styles();
    create_header_footer();

    for(int i = 0; i < 4; i++) {
        s_pages[i] = lv_obj_create(scr);
        lv_obj_set_size(s_pages[i], 240, 102);
        lv_obj_set_pos(s_pages[i], 0, 18);
        lv_obj_add_style(s_pages[i], &s_styleBg, 0);
        lv_obj_add_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_pages[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(s_pages[i], LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_set_style_border_width(s_pages[i], 0, 0);
        lv_obj_set_style_pad_all(s_pages[i], 0, 0);
        lv_obj_set_scrollbar_mode(s_pages[i], LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_outline_width(s_pages[i], 0, 0);
        lv_obj_set_style_outline_width(s_pages[i], 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(s_pages[i], 0, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(s_pages[i], 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    }

    create_page_soc();
    create_page_cccv();
    create_page_discharge();
    create_page_system();
    bms_ui_view_switch_page(0);
}

static void init_styles(void)
{
    lv_style_init(&s_styleBg);
    lv_style_set_bg_color(&s_styleBg, COLOR_BG);
    lv_style_set_bg_opa(&s_styleBg, LV_OPA_COVER);
    lv_style_set_pad_all(&s_styleBg, 0);
    lv_style_set_border_width(&s_styleBg, 0);
    lv_style_set_radius(&s_styleBg, 0);

    lv_style_init(&s_styleTextGold);
    lv_style_set_text_color(&s_styleTextGold, COLOR_GOLD);
    lv_style_set_text_font(&s_styleTextGold, &lv_font_montserrat_12);

    lv_style_init(&s_styleTextCyan);
    lv_style_set_text_color(&s_styleTextCyan, COLOR_CYAN);
    lv_style_set_text_font(&s_styleTextCyan, &lv_font_montserrat_12);

    lv_style_init(&s_styleTextGray);
    lv_style_set_text_color(&s_styleTextGray, COLOR_GRAY);
    lv_style_set_text_font(&s_styleTextGray, &lv_font_montserrat_12);

    lv_style_init(&s_styleBtnNormal);
    lv_style_set_bg_color(&s_styleBtnNormal, COLOR_BG);
    lv_style_set_bg_opa(&s_styleBtnNormal, LV_OPA_COVER);
    lv_style_set_border_width(&s_styleBtnNormal, 1);
    lv_style_set_border_color(&s_styleBtnNormal, COLOR_GRAY);
    lv_style_set_text_color(&s_styleBtnNormal, lv_color_white());
    lv_style_set_text_font(&s_styleBtnNormal, &lv_font_montserrat_12);
    lv_style_set_pad_all(&s_styleBtnNormal, 0);
    lv_style_set_radius(&s_styleBtnNormal, 0);

    lv_style_init(&s_styleBtnFocused);
    lv_style_set_border_width(&s_styleBtnFocused, 1);
    lv_style_set_border_color(&s_styleBtnFocused, COLOR_GRAY);
    lv_style_set_outline_width(&s_styleBtnFocused, 0);
    lv_style_set_bg_color(&s_styleBtnFocused, COLOR_BG);
    lv_style_set_bg_opa(&s_styleBtnFocused, LV_OPA_COVER);
    lv_style_set_radius(&s_styleBtnFocused, 0);

    lv_style_init(&s_styleBtnEditing);
    lv_style_set_bg_color(&s_styleBtnEditing, COLOR_GOLD);
    lv_style_set_bg_opa(&s_styleBtnEditing, LV_OPA_COVER);
    lv_style_set_border_width(&s_styleBtnEditing, 1);
    lv_style_set_border_color(&s_styleBtnEditing, COLOR_GOLD);
    lv_style_set_text_color(&s_styleBtnEditing, COLOR_BG);
    lv_style_set_outline_width(&s_styleBtnEditing, 0);
    lv_style_set_radius(&s_styleBtnEditing, 0);
}

/**********************
 *   HEADER / FOOTER
 **********************/

static void create_header_footer(void)
{
    lv_obj_t* scr = lv_screen_active();

    lv_obj_t* header = lv_obj_create(scr);
    lv_obj_set_size(header, 240, 18);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_add_style(header, &s_styleBg, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_scrollbar_mode(header, LV_SCROLLBAR_MODE_OFF);

    s_lblMode = lv_label_create(header);
    lv_obj_add_style(s_lblMode, &s_styleTextCyan, 0);
    lv_label_set_text(s_lblMode, "[ SoC LNK ]");
    lv_obj_set_pos(s_lblMode, 8, 3);

    s_barSoc = lv_bar_create(header);
    lv_obj_set_size(s_barSoc, 60, 14);
    lv_obj_set_pos(s_barSoc, 115, 2);
    lv_obj_set_style_bg_color(s_barSoc, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(s_barSoc, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_barSoc, 1, 0);
    lv_obj_set_style_border_color(s_barSoc, COLOR_CYAN, 0);
    lv_obj_set_style_pad_all(s_barSoc, 0, 0);
    lv_obj_set_style_bg_color(s_barSoc, COLOR_CYAN, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_barSoc, LV_OPA_50, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_barSoc, 0, 0);
    lv_obj_set_style_radius(s_barSoc, 0, LV_PART_INDICATOR);
    lv_bar_set_value(s_barSoc, 85, LV_ANIM_OFF);

    s_lblSoc = lv_label_create(s_barSoc);
    lv_obj_add_style(s_lblSoc, &s_styleTextCyan, 0);
    lv_obj_set_style_text_color(s_lblSoc, lv_color_white(), 0);
    lv_label_set_text(s_lblSoc, "85%");
    lv_obj_center(s_lblSoc);

    s_lblLink = lv_label_create(header);
    lv_obj_add_style(s_lblLink, &s_styleTextCyan, 0);
    lv_obj_set_pos(s_lblLink, 180, 3);
    update_link_status(true);

    lv_obj_t* line = lv_line_create(scr);
    static lv_point_precise_t line_pts[] = {{0, 18}, {240, 18}};
    lv_line_set_points(line, line_pts, 2);
    lv_obj_set_style_line_width(line, 1, 0);
    lv_obj_set_style_line_color(line, COLOR_GRAY, 0);

    lv_obj_t* footer = lv_obj_create(scr);
    lv_obj_set_size(footer, 240, 15);
    lv_obj_set_pos(footer, 0, 120);
    lv_obj_add_style(footer, &s_styleBg, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, 0, 0);
    lv_obj_set_scrollbar_mode(footer, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* line_f = lv_line_create(scr);
    static lv_point_precise_t line_f_pts[] = {{0, 120}, {240, 120}};
    lv_line_set_points(line_f, line_f_pts, 2);
    lv_obj_set_style_line_width(line_f, 1, 0);
    lv_obj_set_style_line_color(line_f, COLOR_GRAY, 0);

    s_circle = lv_obj_create(footer);
    lv_obj_set_size(s_circle, 8, 8);
    lv_obj_set_pos(s_circle, 8, 3);
    lv_obj_set_style_radius(s_circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(s_circle, 0, 0);
    lv_obj_set_style_pad_all(s_circle, 0, 0);
    lv_obj_set_style_shadow_width(s_circle, 0, 0);
    lv_obj_set_style_outline_width(s_circle, 0, 0);
    lv_obj_set_style_bg_color(s_circle, COLOR_GOLD, 0);
    lv_obj_set_style_bg_opa(s_circle, LV_OPA_COVER, 0);

    s_btnFooter = lv_button_create(footer);
    lv_obj_set_size(s_btnFooter, 70, 13);
    lv_obj_set_pos(s_btnFooter, 165, 1);
    lv_obj_set_style_radius(s_btnFooter, 0, 0);
    lv_obj_set_style_border_width(s_btnFooter, 0, 0);
    lv_obj_set_style_shadow_width(s_btnFooter, 0, 0);
    lv_obj_set_style_outline_width(s_btnFooter, 0, 0);
    lv_obj_set_style_pad_all(s_btnFooter, 0, 0);
    lv_obj_set_style_bg_opa(s_btnFooter, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_btnFooter, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(s_btnFooter, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(s_btnFooter, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(s_btnFooter, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(s_btnFooter, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(s_btnFooter, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_add_style(s_btnFooter, &s_styleBtnEditing, LV_STATE_USER_1);
    lv_obj_add_style(s_btnFooter, &s_styleBtnEditing, LV_STATE_USER_1 | LV_STATE_FOCUS_KEY);

    s_lblFooter = lv_label_create(s_btnFooter);
    lv_obj_add_style(s_lblFooter, &s_styleTextCyan, 0);
    lv_label_set_text(s_lblFooter, "[ * - - - ]");
    lv_obj_center(s_lblFooter);
}

/**********************
 *   PAGE CREATION
 **********************/

static void setup_hud_button(lv_obj_t* btn)
{
    lv_obj_set_size(btn, 85, 22);
    lv_obj_add_style(btn, &s_styleBtnNormal, 0);
    lv_obj_add_style(btn, &s_styleBtnFocused, LV_STATE_FOCUSED);
    lv_obj_add_style(btn, &s_styleBtnFocused, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn, &s_styleBtnFocused, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn, &s_styleBtnEditing, LV_STATE_USER_1);
    lv_obj_add_style(btn, &s_styleBtnEditing, LV_STATE_USER_1 | LV_STATE_FOCUS_KEY);

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

static void make_seg(lv_obj_t* bar, int w, int x, lv_color_t c)
{
    if(w <= 0) return;
    lv_obj_t* s = lv_obj_create(bar);
    lv_obj_set_size(s, w, 22);
    lv_obj_set_pos(s, x, 0);
    strip_decorations(s);
    lv_obj_set_style_bg_color(s, c, 0);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
}

static lv_obj_t* create_selection_bar(lv_obj_t* parent, int width)
{
    lv_obj_t* bar = lv_obj_create(parent);
    lv_obj_set_size(bar, width, 22);
    lv_obj_set_pos(bar, 0, 0);
    strip_decorations(bar);
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

static void create_page_soc(void)
{
    lv_obj_t* p = s_pages[0];

    s_lblSocBig = lv_label_create(p);
    lv_obj_set_style_text_font(s_lblSocBig, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_lblSocBig, COLOR_GOLD, 0);
    lv_label_set_text(s_lblSocBig, "85%");
    lv_obj_set_pos(s_lblSocBig, 10, 10);

    s_lblTrend = lv_label_create(p);
    lv_obj_add_style(s_lblTrend, &s_styleTextGray, 0);
    lv_label_set_text(s_lblTrend, "[STANDBY]");
    lv_obj_set_pos(s_lblTrend, 10, 44);

    lv_obj_t* desc = lv_label_create(p);
    lv_obj_add_style(desc, &s_styleTextGray, 0);
    lv_obj_set_style_text_font(desc, &lv_font_montserrat_12, 0);
    lv_label_set_text(desc, "CHIRAL EST. SOC");
    lv_obj_set_pos(desc, 10, 66);

    s_lblU = lv_label_create(p);
    lv_obj_add_style(s_lblU, &s_styleTextCyan, 0);
    lv_label_set_text(s_lblU, "U: 4.120 V");
    lv_obj_set_pos(s_lblU, 125, 12);

    s_lblI = lv_label_create(p);
    lv_obj_add_style(s_lblI, &s_styleTextCyan, 0);
    lv_label_set_text(s_lblI, "I:  0.00 A");
    lv_obj_set_pos(s_lblI, 125, 32);

    s_lblT = lv_label_create(p);
    lv_obj_add_style(s_lblT, &s_styleTextCyan, 0);
    lv_label_set_text(s_lblT, "T: 25.0 C");
    lv_obj_set_pos(s_lblT, 125, 52);

    s_lblR = lv_label_create(p);
    lv_obj_add_style(s_lblR, &s_styleTextGray, 0);
    lv_label_set_text(s_lblR, "R: 0.05 Ohm");
    lv_obj_set_pos(s_lblR, 125, 72);
}

static void create_page_cccv(void)
{
    lv_obj_t* p = s_pages[1];

    s_btnUset = lv_button_create(p);
    lv_obj_set_pos(s_btnUset, 4, 8);
    setup_hud_button(s_btnUset);
    lv_obj_t* lbl = lv_label_create(s_btnUset);
    lv_label_set_text(lbl, "U: 4.20V");
    lv_obj_center(lbl);

    s_btnIset = lv_button_create(p);
    lv_obj_set_pos(s_btnIset, 4, 36);
    setup_hud_button(s_btnIset);
    lbl = lv_label_create(s_btnIset);
    lv_label_set_text(lbl, "I: 2.00A");
    lv_obj_center(lbl);

    s_btnChgToggle = lv_button_create(p);
    lv_obj_set_pos(s_btnChgToggle, 4, 64);
    setup_hud_button(s_btnChgToggle);
    bms_ui_view_update_toggle_style(s_btnChgToggle, false);
    lbl = lv_label_create(s_btnChgToggle);
    lv_label_set_text(lbl, "CHG: OFF");
    lv_obj_center(lbl);

    s_chartP2 = lv_chart_create(p);
    lv_obj_set_size(s_chartP2, 136, 56);
    lv_obj_set_pos(s_chartP2, 98, 8);
    lv_chart_set_type(s_chartP2, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_chartP2, 20);
    lv_chart_set_range(s_chartP2, LV_CHART_AXIS_PRIMARY_Y, 0, 5000);
    lv_obj_set_style_bg_color(s_chartP2, COLOR_BG, 0);
    lv_obj_set_style_border_color(s_chartP2, COLOR_DARK_GRAY, 0);
    lv_obj_set_style_border_width(s_chartP2, 1, 0);
    lv_obj_set_style_pad_all(s_chartP2, 0, 0);
    lv_obj_set_style_line_color(s_chartP2, COLOR_DARK_GRAY, LV_PART_ITEMS);
    lv_obj_set_style_width(s_chartP2, 0, LV_PART_INDICATOR);
    lv_obj_set_style_height(s_chartP2, 0, LV_PART_INDICATOR);

    s_p2SerU = lv_chart_add_series(s_chartP2, COLOR_CYAN, LV_CHART_AXIS_PRIMARY_Y);
    s_p2SerI = lv_chart_add_series(s_chartP2, COLOR_GOLD, LV_CHART_AXIS_PRIMARY_Y);

    for(int i = 0; i < 20; i++) {
        lv_chart_set_next_value(s_chartP2, s_p2SerU, 0);
        lv_chart_set_next_value(s_chartP2, s_p2SerI, 0);
    }

    s_lblP2Readout = lv_label_create(p);
    lv_obj_add_style(s_lblP2Readout, &s_styleTextGray, 0);
    lv_obj_set_style_text_font(s_lblP2Readout, &lv_font_montserrat_12, 0);
    lv_label_set_text(s_lblP2Readout, "U: 4.12V  I: 0.00A");
    lv_obj_set_pos(s_lblP2Readout, 98, 68);
}

static void create_page_discharge(void)
{
    lv_obj_t* p = s_pages[2];

    s_btnIdis = lv_button_create(p);
    lv_obj_set_pos(s_btnIdis, 4, 20);
    setup_hud_button(s_btnIdis);
    lv_obj_t* lbl = lv_label_create(s_btnIdis);
    lv_label_set_text(lbl, "I: 3.00A");
    lv_obj_center(lbl);

    s_btnDscToggle = lv_button_create(p);
    lv_obj_set_pos(s_btnDscToggle, 4, 52);
    setup_hud_button(s_btnDscToggle);
    bms_ui_view_update_toggle_style(s_btnDscToggle, false);
    lbl = lv_label_create(s_btnDscToggle);
    lv_label_set_text(lbl, "DSC: OFF");
    lv_obj_center(lbl);

    s_chartP3 = lv_chart_create(p);
    lv_obj_set_size(s_chartP3, 136, 56);
    lv_obj_set_pos(s_chartP3, 98, 8);
    lv_chart_set_type(s_chartP3, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_chartP3, 20);
    lv_chart_set_range(s_chartP3, LV_CHART_AXIS_PRIMARY_Y, 0, 5000);
    lv_obj_set_style_bg_color(s_chartP3, COLOR_BG, 0);
    lv_obj_set_style_border_color(s_chartP3, COLOR_DARK_GRAY, 0);
    lv_obj_set_style_border_width(s_chartP3, 1, 0);
    lv_obj_set_style_pad_all(s_chartP3, 0, 0);
    lv_obj_set_style_line_color(s_chartP3, COLOR_DARK_GRAY, LV_PART_ITEMS);
    lv_obj_set_style_width(s_chartP3, 0, LV_PART_INDICATOR);
    lv_obj_set_style_height(s_chartP3, 0, LV_PART_INDICATOR);

    s_p3SerU = lv_chart_add_series(s_chartP3, COLOR_CYAN, LV_CHART_AXIS_PRIMARY_Y);
    s_p3SerI = lv_chart_add_series(s_chartP3, COLOR_GOLD, LV_CHART_AXIS_PRIMARY_Y);

    for(int i = 0; i < 20; i++) {
        lv_chart_set_next_value(s_chartP3, s_p3SerU, 0);
        lv_chart_set_next_value(s_chartP3, s_p3SerI, 0);
    }

    s_lblP3Readout = lv_label_create(p);
    lv_obj_add_style(s_lblP3Readout, &s_styleTextGray, 0);
    lv_obj_set_style_text_font(s_lblP3Readout, &lv_font_montserrat_12, 0);
    lv_label_set_text(s_lblP3Readout, "U: 4.12V  I: 0.00A");
    lv_obj_set_pos(s_lblP3Readout, 98, 68);
}

static void create_page_system(void)
{
    lv_obj_t* p = s_pages[3];

    s_btnBaud = lv_button_create(p);
    lv_obj_set_pos(s_btnBaud, 4, 20);
    setup_hud_button(s_btnBaud);
    lv_obj_t* lbl = lv_label_create(s_btnBaud);
    lv_label_set_text(lbl, "Baud:115K");
    lv_obj_center(lbl);

    s_btnPort = lv_button_create(p);
    lv_obj_set_pos(s_btnPort, 4, 52);
    setup_hud_button(s_btnPort);
    lbl = lv_label_create(s_btnPort);
    lv_label_set_text(lbl, "Port:UART0");
    lv_obj_center(lbl);

    s_lblTerminal = lv_label_create(p);
    lv_obj_add_style(s_lblTerminal, &s_styleTextGold, 0);
    lv_obj_set_style_text_font(s_lblTerminal, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(s_lblTerminal, 98, 8);
    lv_obj_set_size(s_lblTerminal, 136, 80);
    lv_label_set_long_mode(s_lblTerminal, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_radius(s_lblTerminal, 4, 0);
    lv_obj_set_style_bg_color(s_lblTerminal, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(s_lblTerminal, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_lblTerminal, 1, 0);
    lv_obj_set_style_border_color(s_lblTerminal, lv_color_black(), 0);
    lv_obj_set_style_pad_all(s_lblTerminal, 3, 0);
}

/**********************
 *   PAGE SWITCHING
 **********************/

void bms_ui_view_update_footer_label(int page, bool bracketed)
{
    if(!s_lblFooter) return;
    const char* labels[] = {"* - - -", "- * - -", "- - * -", "- - - *"};
    if(page < 0 || page > 3) return;
    char buf[20];
    if(bracketed) snprintf(buf, sizeof(buf), "[ %s ]", labels[page]);
    else          snprintf(buf, sizeof(buf), "%s", labels[page]);
    lv_label_set_text(s_lblFooter, buf);
}

void bms_ui_view_switch_page(int page_idx)
{
    if(page_idx < 0 || page_idx >= 4) return;
    s_switchingPage = true;

    bool keep_footer = false;
    lv_group_t* g = lv_group_get_default();
    if(g && s_btnFooter && lv_group_get_focused(g) == s_btnFooter) keep_footer = true;

    for(int i = 0; i < 4; i++) lv_obj_add_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_pages[page_idx], LV_OBJ_FLAG_HIDDEN);

    if(s_lblFooter) {
        bool focused = (s_btnFooter && lv_obj_has_state(s_btnFooter, LV_STATE_FOCUSED));
        bms_ui_view_update_footer_label(page_idx, !(focused && s_footerSelected));
    }

    if(s_lblMode) {
        const char* texts[] = {"[ SoC LNK ]", "[ CCCV CHG ]", "[ CC DISCH ]", "[ SYS REG ]"};
        lv_label_set_text(s_lblMode, texts[page_idx]);
    }

    if(g) {
        lv_group_set_editing(g, false);
        if(s_btnUset) lv_obj_remove_state(s_btnUset, LV_STATE_USER_1);
        if(s_btnIset) lv_obj_remove_state(s_btnIset, LV_STATE_USER_1);
        if(s_btnIdis) lv_obj_remove_state(s_btnIdis, LV_STATE_USER_1);
        if(s_btnBaud) lv_obj_remove_state(s_btnBaud, LV_STATE_USER_1);
        if(s_btnPort) lv_obj_remove_state(s_btnPort, LV_STATE_USER_1);

        lv_group_remove_all_objs(g);

        switch(page_idx) {
            case 0:
                lv_group_add_obj(g, s_btnFooter);
                lv_group_focus_obj(s_btnFooter);
                break;
            case 1:
                lv_group_add_obj(g, s_btnUset);
                lv_group_add_obj(g, s_btnIset);
                lv_group_add_obj(g, s_btnChgToggle);
                lv_group_add_obj(g, s_btnFooter);
                lv_group_focus_obj(keep_footer ? s_btnFooter : s_btnUset);
                break;
            case 2:
                lv_group_add_obj(g, s_btnIdis);
                lv_group_add_obj(g, s_btnDscToggle);
                lv_group_add_obj(g, s_btnFooter);
                lv_group_focus_obj(keep_footer ? s_btnFooter : s_btnIdis);
                break;
            case 3:
                lv_group_add_obj(g, s_btnBaud);
                lv_group_add_obj(g, s_btnPort);
                lv_group_add_obj(g, s_btnFooter);
                lv_group_focus_obj(keep_footer ? s_btnFooter : s_btnBaud);
                break;
        }
    }

    s_currentPage = page_idx;
    s_switchingPage = false;
}

/**********************
 *   STYLE UPDATES
 **********************/

void bms_ui_view_update_toggle_style(lv_obj_t* btn, bool active)
{
    if(!btn) return;
    lv_obj_t* bar = bms_ui_view_get_button_bar(btn);

    if(active) {
        lv_obj_set_style_bg_color(btn, COLOR_BG, 0);
        lv_obj_set_style_bg_color(btn, COLOR_BG, LV_STATE_FOCUSED);
        lv_obj_set_style_bg_color(btn, COLOR_BG, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_bg_color(btn, COLOR_BG, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_FOCUSED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);

        if(bar) {
            lv_obj_remove_flag(bar, LV_OBJ_FLAG_HIDDEN);
            lv_obj_t* s1 = lv_obj_get_child(bar, 0);
            lv_obj_t* s2 = lv_obj_get_child(bar, 1);
            lv_obj_t* s3 = lv_obj_get_child(bar, 2);
            lv_obj_t* s4 = lv_obj_get_child(bar, 3);
            if(s1) lv_obj_set_style_bg_color(s1, COLOR_GOLD, 0);
            if(s2) lv_obj_set_style_bg_color(s2, lv_color_make(0xC0, 0x77, 0x00), 0);
            if(s3) lv_obj_set_style_bg_color(s3, lv_color_make(0x80, 0x50, 0x00), 0);
            if(s4) lv_obj_set_style_bg_color(s4, lv_color_make(0x40, 0x27, 0x00), 0);
        }
    } else {
        lv_obj_set_style_bg_color(btn, COLOR_CYAN, 0);
        lv_obj_set_style_bg_color(btn, COLOR_BG, LV_STATE_FOCUSED);
        lv_obj_set_style_bg_color(btn, COLOR_BG, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_bg_color(btn, COLOR_BG, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_FOCUSED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);

        if(bar) {
            lv_obj_t* s1 = lv_obj_get_child(bar, 0);
            lv_obj_t* s2 = lv_obj_get_child(bar, 1);
            lv_obj_t* s3 = lv_obj_get_child(bar, 2);
            lv_obj_t* s4 = lv_obj_get_child(bar, 3);
            if(s1) lv_obj_set_style_bg_color(s1, COLOR_HUD_BLUE, 0);
            if(s2) lv_obj_set_style_bg_color(s2, COLOR_DARK_BLUE_1, 0);
            if(s3) lv_obj_set_style_bg_color(s3, COLOR_DARK_BLUE_2, 0);
            if(s4) lv_obj_set_style_bg_color(s4, COLOR_DARK_BLUE_3, 0);
        }
    }

    lv_obj_set_style_text_color(btn, lv_color_white(), 0);
    lv_obj_set_style_text_color(btn, lv_color_white(), LV_STATE_FOCUSED);
    lv_obj_set_style_text_color(btn, lv_color_white(), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_color(btn, lv_color_white(), LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
}

static void update_link_status(bool online)
{
    if(!s_lblLink) return;
    if(online) {
        lv_label_set_text(s_lblLink, "ONLINE");
        lv_obj_set_style_text_color(s_lblLink, COLOR_DS_TEAL, 0);
    } else {
        lv_label_set_text(s_lblLink, "OFFLINE");
        lv_obj_set_style_text_color(s_lblLink, COLOR_RED, 0);
    }
}

/**********************
 *   REFRESH
 **********************/

void bms_ui_view_update_soc(uint8_t soc)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", soc);
    if(s_lblSoc) { lv_label_set_text(s_lblSoc, buf); lv_obj_center(s_lblSoc); }
    if(s_barSoc) lv_bar_set_value(s_barSoc, soc, LV_ANIM_OFF);
    if(s_lblSocBig && s_currentPage == 0) lv_label_set_text(s_lblSocBig, buf);
}

void bms_ui_view_refresh(const bms_state_t* s)
{
    if(!s) return;

    if(s_currentPage == 0) {
        char buf[24];
        float u = s->voltage_mV / 1000.0f;
        float i = s->current_mA / 1000.0f;
        float t = s->temperature_x10 / 10.0f;

        snprintf(buf, sizeof(buf), "U: %.3f V", u);
        lv_label_set_text(s_lblU, buf);
        snprintf(buf, sizeof(buf), "I: %+.2f A", i);
        lv_label_set_text(s_lblI, buf);
        snprintf(buf, sizeof(buf), "T: %.1f C", t);
        lv_label_set_text(s_lblT, buf);

        if(s->charge_active) {
            lv_label_set_text(s_lblTrend, "[+ CHARGING]");
            lv_obj_set_style_text_color(s_lblTrend, COLOR_GOLD, 0);
        } else if(s->discharge_active) {
            lv_label_set_text(s_lblTrend, "[- DISCHG]");
            lv_obj_set_style_text_color(s_lblTrend, COLOR_RED, 0);
        } else if(s->low_volt_alert) {
            lv_label_set_text(s_lblTrend, "[! L-VOLT]");
            lv_obj_set_style_text_color(s_lblTrend, COLOR_RED, 0);
        } else {
            lv_label_set_text(s_lblTrend, "[STANDBY]");
            lv_obj_set_style_text_color(s_lblTrend, COLOR_GRAY, 0);
        }
    }

    if(s_currentPage == 1) {
        char buf[36];
        float u = s->voltage_mV / 1000.0f;
        float i = s->charge_active ? (s->current_mA / 1000.0f) : 0.0f;
        snprintf(buf, sizeof(buf), "U:%.2fV I:%.2fA P:%.2fW", u, i, u * i);
        lv_label_set_text(s_lblP2Readout, buf);
        lv_chart_set_next_value(s_chartP2, s_p2SerU, s->voltage_mV);
        lv_chart_set_next_value(s_chartP2, s_p2SerI, s->charge_active ? s->current_mA : 0);
    }

    if(s_currentPage == 2) {
        char buf[36];
        if(s->low_volt_alert) {
            snprintf(buf, sizeof(buf), "CUTOFF! U < 2.80V");
            lv_label_set_text(s_lblP3Readout, buf);
            lv_obj_set_style_text_color(s_lblP3Readout, COLOR_RED, 0);
        } else {
            float u = s->voltage_mV / 1000.0f;
            float i = s->discharge_active ? (-s->current_mA / 1000.0f) : 0.0f;
            snprintf(buf, sizeof(buf), "U:%.2fV I:%.2fA P:%.2fW", u, i, u * i);
            lv_label_set_text(s_lblP3Readout, buf);
            lv_obj_set_style_text_color(s_lblP3Readout, COLOR_GRAY, 0);
        }
        lv_chart_set_next_value(s_chartP3, s_p3SerU, s->voltage_mV);
        lv_chart_set_next_value(s_chartP3, s_p3SerI, s->discharge_active ? (-s->current_mA / 2) : 0);
    }

    if(s_currentPage == 3) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s\n%s\n%s\n%s",
                 s->log_lines[0], s->log_lines[1], s->log_lines[2], s->log_lines[3]);
        lv_label_set_text(s_lblTerminal, buf);
    }

    /* Auto-stopped toggle label updates */
    if(s->charge_auto_stopped && s_btnChgToggle) {
        lv_label_set_text(bms_ui_view_get_button_label(s_btnChgToggle), "CHG: OFF");
        bms_ui_view_update_toggle_style(s_btnChgToggle, false);
    }
    if(s->discharge_auto_stopped && s_btnDscToggle) {
        lv_label_set_text(bms_ui_view_get_button_label(s_btnDscToggle), "DSC: OFF");
        bms_ui_view_update_toggle_style(s_btnDscToggle, false);
    }

    update_link_status(s->host_online);
    bms_ui_view_update_soc(s->predicted_soc);

    /* Footer circle */
    if(s_circle) {
        if(s_currentPage == 0 || s_currentPage == 3) {
            lv_obj_set_style_bg_color(s_circle, COLOR_GOLD, 0);
            lv_obj_set_style_bg_opa(s_circle, LV_OPA_COVER, 0);
        } else {
            bool blink_on = (lv_tick_get() % 1000) < 500;
            if(blink_on) {
                bool active = (s_currentPage == 1) ? s->charge_active : s->discharge_active;
                lv_obj_set_style_bg_color(s_circle, active ? COLOR_RED : COLOR_DS_TEAL, 0);
                lv_obj_set_style_bg_opa(s_circle, LV_OPA_COVER, 0);
            } else {
                lv_obj_set_style_bg_opa(s_circle, LV_OPA_TRANSP, 0);
            }
        }
    }
}
