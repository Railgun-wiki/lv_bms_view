#include "bms_ui_view.h"
#include "bms_ui_internal.h"
#include "bms_ui_styles.h"
#include "bms_ui_pages.h"
#include <stdio.h>
#include <string.h>

/**********************
 *  STATIC VARIABLES
 **********************/
static bms_ui_widgets_t s_w;
static int s_currentPage = 0;
static bool s_switchingPage = false;
static bool s_footerSelected = false;

/**********************
 *  UTILITY (shared)
 **********************/

void bms_ui_strip_decorations(lv_obj_t* obj)
{
    if(!obj) return;
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t* bms_ui_get_button_label(lv_obj_t* btn)
{
    if(!btn) return NULL;
    uint32_t cnt = lv_obj_get_child_cnt(btn);
    for(uint32_t i = 0; i < cnt; i++) {
        lv_obj_t* child = lv_obj_get_child(btn, i);
        if(child && lv_obj_check_type(child, &lv_label_class)) return child;
    }
    return NULL;
}

lv_obj_t* bms_ui_get_button_bar(lv_obj_t* btn)
{
    if(!btn) return NULL;
    uint32_t cnt = lv_obj_get_child_cnt(btn);
    for(uint32_t i = 0; i < cnt; i++) {
        lv_obj_t* child = lv_obj_get_child(btn, i);
        if(child && !lv_obj_check_type(child, &lv_label_class)) return child;
    }
    return NULL;
}

/**********************
 *  GETTERS
 **********************/

int  bms_ui_view_current_page(void)         { return s_currentPage; }
bool bms_ui_view_footer_selected(void)       { return s_footerSelected; }
void bms_ui_view_set_footer_selected(bool v) { s_footerSelected = v; }
bool bms_ui_view_switching_page(void)        { return s_switchingPage; }

lv_obj_t* bms_ui_view_btn_charge_toggle(void)    { return s_w.btnChgToggle; }
lv_obj_t* bms_ui_view_btn_discharge_toggle(void)  { return s_w.btnDscToggle; }
lv_obj_t* bms_ui_view_footer_menu(void)           { return s_w.btnFooter; }
lv_obj_t* bms_ui_view_btn_uset(void)              { return s_w.btnUset; }
lv_obj_t* bms_ui_view_btn_iset(void)              { return s_w.btnIset; }
lv_obj_t* bms_ui_view_btn_idis(void)              { return s_w.btnIdis; }
lv_obj_t* bms_ui_view_btn_baud(void)              { return s_w.btnBaud; }
lv_obj_t* bms_ui_view_btn_port(void)              { return s_w.btnPort; }

lv_obj_t* bms_ui_view_get_button_label(lv_obj_t* btn) { return bms_ui_get_button_label(btn); }
lv_obj_t* bms_ui_view_get_button_bar(lv_obj_t* btn)   { return bms_ui_get_button_bar(btn); }

/**********************
 *  INIT
 **********************/

void bms_ui_view_init(void)
{
    memset(&s_w, 0, sizeof(s_w));

    lv_group_t* g = lv_group_get_default();
    if(g) lv_group_remove_all_objs(g);
    else { g = lv_group_create(); lv_group_set_default(g); }

    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    bms_ui_styles_init();
    bms_ui_pages_create(&s_w);
    bms_ui_view_switch_page(0);
}

/**********************
 *   PAGE SWITCHING
 **********************/

void bms_ui_view_update_footer_label(int page, bool bracketed)
{
    if(!s_w.lblFooter) return;
    const char* labels[] = {"* - - -", "- * - -", "- - * -", "- - - *"};
    if(page < 0 || page > 3) return;
    char buf[20];
    if(bracketed) snprintf(buf, sizeof(buf), "[ %s ]", labels[page]);
    else          snprintf(buf, sizeof(buf), "%s", labels[page]);
    lv_label_set_text(s_w.lblFooter, buf);
}

void bms_ui_view_switch_page(int page_idx)
{
    if(page_idx < 0 || page_idx >= 4) return;
    s_switchingPage = true;

    bool keep_footer = false;
    lv_group_t* g = lv_group_get_default();
    if(g && s_w.btnFooter && lv_group_get_focused(g) == s_w.btnFooter) keep_footer = true;

    for(int i = 0; i < 4; i++) lv_obj_add_flag(s_w.pages[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_w.pages[page_idx], LV_OBJ_FLAG_HIDDEN);

    if(s_w.lblFooter) {
        bool focused = (s_w.btnFooter && lv_obj_has_state(s_w.btnFooter, LV_STATE_FOCUSED));
        bms_ui_view_update_footer_label(page_idx, !(focused && s_footerSelected));
    }

    if(s_w.lblMode) {
        const char* texts[] = {"[ SoC LNK ]", "[ CCCV CHG ]", "[ CC DISCH ]", "[ SYS REG ]"};
        lv_label_set_text(s_w.lblMode, texts[page_idx]);
    }

    if(g) {
        lv_group_set_editing(g, false);
        if(s_w.btnUset) lv_obj_remove_state(s_w.btnUset, LV_STATE_USER_1);
        if(s_w.btnIset) lv_obj_remove_state(s_w.btnIset, LV_STATE_USER_1);
        if(s_w.btnIdis) lv_obj_remove_state(s_w.btnIdis, LV_STATE_USER_1);
        if(s_w.btnBaud) lv_obj_remove_state(s_w.btnBaud, LV_STATE_USER_1);
        if(s_w.btnPort) lv_obj_remove_state(s_w.btnPort, LV_STATE_USER_1);

        lv_group_remove_all_objs(g);

        lv_obj_t* focus_target = NULL;
        switch(page_idx) {
            case 0:
                lv_group_add_obj(g, s_w.btnFooter);
                focus_target = s_w.btnFooter;
                break;
            case 1:
                lv_group_add_obj(g, s_w.btnUset);
                lv_group_add_obj(g, s_w.btnIset);
                lv_group_add_obj(g, s_w.btnChgToggle);
                lv_group_add_obj(g, s_w.btnFooter);
                focus_target = keep_footer ? s_w.btnFooter : s_w.btnUset;
                break;
            case 2:
                lv_group_add_obj(g, s_w.btnIdis);
                lv_group_add_obj(g, s_w.btnDscToggle);
                lv_group_add_obj(g, s_w.btnFooter);
                focus_target = keep_footer ? s_w.btnFooter : s_w.btnIdis;
                break;
            case 3:
                lv_group_add_obj(g, s_w.btnBaud);
                lv_group_add_obj(g, s_w.btnPort);
                lv_group_add_obj(g, s_w.btnFooter);
                focus_target = keep_footer ? s_w.btnFooter : s_w.btnBaud;
                break;
        }
        if(focus_target) {
            lv_group_focus_obj(focus_target);
            /* Apply focus styling immediately (lv_group_focus_obj alone
               doesn't trigger LV_EVENT_FOCUSED until an input event) */
            if(focus_target == s_w.btnFooter) {
                lv_obj_set_style_bg_color(s_w.btnFooter, lv_color_make(0x00, 0x66, 0xBB), 0);
                lv_obj_set_style_bg_opa(s_w.btnFooter, LV_OPA_COVER, 0);
                if(s_w.lblFooter) lv_obj_set_style_text_color(s_w.lblFooter, lv_color_white(), 0);
            }
        }
    }

    s_currentPage = page_idx;
    s_switchingPage = false;
}

/**********************
 *  PUBLIC REFRESH WRAPPERS
 **********************/

void bms_ui_view_update_soc(uint8_t soc)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", soc);
    if(s_w.lblSoc) { lv_label_set_text(s_w.lblSoc, buf); lv_obj_center(s_w.lblSoc); }
    if(s_w.barSoc) lv_bar_set_value(s_w.barSoc, soc, LV_ANIM_OFF);
    if(s_w.lblSocBig && s_currentPage == 0) lv_label_set_text(s_w.lblSocBig, buf);
}

void bms_ui_view_refresh(const bms_state_t* state)
{
    bms_ui_view_refresh_impl(&s_w, state, s_currentPage);
}
