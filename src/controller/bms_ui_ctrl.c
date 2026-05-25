#include "bms_ui_ctrl.h"
#include "bms_ui_view.h"
#include <stdio.h>

static bms_data_bridge_t s_bridge;

static void fmt_milli(char* buf, int32_t val)
{
    int32_t whole = val / 1000;
    int32_t frac = (val < 0 ? -val : val) % 1000 / 10;
    snprintf(buf, 12, "%ld.%02ld", (long)whole, (long)frac);
}

static void on_click(lv_event_t* e);
static void on_key(lv_event_t* e);
static void on_global_key(lv_event_t* e);
static void on_button_focus(lv_event_t* e);
static void on_footer_focus(lv_event_t* e);

static void bind_btn(lv_obj_t* btn)
{
    if(!btn) return;
    lv_obj_add_event_cb(btn, on_click, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn, on_key, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(btn, on_button_focus, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(btn, on_button_focus, LV_EVENT_DEFOCUSED, NULL);
}

void bms_ui_ctrl_init(const bms_data_bridge_t* bridge)
{
    s_bridge = *bridge;

    lv_obj_add_event_cb(bms_ui_view_footer_menu(), on_click, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(bms_ui_view_footer_menu(), on_key, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(bms_ui_view_footer_menu(), on_footer_focus, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(bms_ui_view_footer_menu(), on_footer_focus, LV_EVENT_DEFOCUSED, NULL);

    bind_btn(bms_ui_view_btn_uset());
    bind_btn(bms_ui_view_btn_iset());
    bind_btn(bms_ui_view_btn_charge_toggle());
    bind_btn(bms_ui_view_btn_idis());
    bind_btn(bms_ui_view_btn_discharge_toggle());
    bind_btn(bms_ui_view_btn_baud());
    bind_btn(bms_ui_view_btn_port());

    lv_obj_add_event_cb(lv_screen_active(), on_global_key, LV_EVENT_KEY, NULL);
}

/**********************
 *   CLICK HANDLER
 **********************/

static void on_click(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_group_t* g = lv_group_get_default();
    if(!g) return;

    const bms_state_t* st = s_bridge.get_state();

    if(st->charge_active && obj != bms_ui_view_btn_charge_toggle()) return;
    if(st->discharge_active && obj != bms_ui_view_btn_discharge_toggle()) return;

    if(obj == bms_ui_view_btn_uset() || obj == bms_ui_view_btn_iset() ||
       obj == bms_ui_view_btn_idis() || obj == bms_ui_view_btn_baud() ||
       obj == bms_ui_view_btn_port()) {
        if(lv_group_get_editing(g)) {
            lv_group_set_editing(g, false);
            lv_obj_remove_state(obj, LV_STATE_USER_1);
            lv_obj_t* bar = bms_ui_view_get_button_bar(obj);
            if(bar) lv_obj_remove_flag(bar, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_group_set_editing(g, true);
            lv_obj_add_state(obj, LV_STATE_USER_1);
            lv_obj_t* bar = bms_ui_view_get_button_bar(obj);
            if(bar) lv_obj_add_flag(bar, LV_OBJ_FLAG_HIDDEN);
        }
    }
    else if(obj == bms_ui_view_btn_charge_toggle()) {
        s_bridge.set_charge_enable(!st->charge_active);
        st = s_bridge.get_state();
        lv_label_set_text(bms_ui_view_get_button_label(obj), st->charge_active ? "CHG: ON" : "CHG: OFF");
        bms_ui_view_update_toggle_style(obj, st->charge_active);
        lv_obj_t* dsc = bms_ui_view_btn_discharge_toggle();
        if(dsc) {
            lv_label_set_text(bms_ui_view_get_button_label(dsc), "DSC: OFF");
            bms_ui_view_update_toggle_style(dsc, false);
        }
    }
    else if(obj == bms_ui_view_btn_discharge_toggle()) {
        s_bridge.set_discharge_enable(!st->discharge_active);
        st = s_bridge.get_state();
        lv_label_set_text(bms_ui_view_get_button_label(obj), st->discharge_active ? "DSC: ON" : "DSC: OFF");
        bms_ui_view_update_toggle_style(obj, st->discharge_active);
        lv_obj_t* chg = bms_ui_view_btn_charge_toggle();
        if(chg) {
            lv_label_set_text(bms_ui_view_get_button_label(chg), "CHG: OFF");
            bms_ui_view_update_toggle_style(chg, false);
        }
    }
    else if(obj == bms_ui_view_footer_menu()) {
        if(!bms_ui_view_footer_selected() && (!g || !lv_group_get_editing(g))) {
            bms_ui_view_set_footer_selected(true);
            lv_obj_add_state(obj, LV_STATE_USER_1);
            if(g) lv_group_set_editing(g, true);
            bms_ui_view_update_footer_label(bms_ui_view_current_page(), false);
        }
    }
}

/**********************
 *   KEY HANDLER
 **********************/

static void on_key(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_key_t key = lv_event_get_key(e);
    lv_group_t* g = lv_group_get_default();
    if(!g) return;

    const bms_state_t* st = s_bridge.get_state();

    if(st->charge_active && obj == bms_ui_view_btn_charge_toggle()) {
        if(key != LV_KEY_ENTER) return;
    }
    if(st->discharge_active && obj == bms_ui_view_btn_discharge_toggle()) {
        if(key != LV_KEY_ENTER) return;
    }

    /* Footer: LEFT/RIGHT flip pages, ENTER/ESC exit */
    if(obj == bms_ui_view_footer_menu() && bms_ui_view_footer_selected()) {
        if(key == LV_KEY_LEFT) {
            bms_ui_view_switch_page((bms_ui_view_current_page() - 1 + 4) % 4);
            lv_group_focus_obj(obj);
            bms_ui_view_update_footer_label(bms_ui_view_current_page(), false);
            return;
        } else if(key == LV_KEY_RIGHT) {
            bms_ui_view_switch_page((bms_ui_view_current_page() + 1) % 4);
            lv_group_focus_obj(obj);
            bms_ui_view_update_footer_label(bms_ui_view_current_page(), false);
            return;
        } else if(key == LV_KEY_ESC || key == LV_KEY_ENTER) {
            bms_ui_view_set_footer_selected(false);
            lv_obj_remove_state(obj, LV_STATE_USER_1);
            lv_group_set_editing(g, false);
            bms_ui_view_update_footer_label(bms_ui_view_current_page(), true);
            return;
        }
    }

    if(lv_group_get_editing(g)) {
        /* Edit mode: adjust values */
        if(key == LV_KEY_UP || key == LV_KEY_RIGHT) {
            if(obj == bms_ui_view_btn_uset()) {
                s_bridge.set_charge_u_mV(st->charge_u_set_mV + 10);
                char buf[20], tmp[12];
                fmt_milli(tmp, s_bridge.get_state()->charge_u_set_mV);
                snprintf(buf, sizeof(buf), "U: %sV", tmp);
                lv_label_set_text(bms_ui_view_get_button_label(obj), buf);
            }
            else if(obj == bms_ui_view_btn_iset()) {
                s_bridge.set_charge_i_mA(st->charge_i_set_mA + 50);
                char buf[20], tmp[12];
                fmt_milli(tmp, s_bridge.get_state()->charge_i_set_mA);
                snprintf(buf, sizeof(buf), "I: %sA", tmp);
                lv_label_set_text(bms_ui_view_get_button_label(obj), buf);
            }
            else if(obj == bms_ui_view_btn_idis()) {
                s_bridge.set_discharge_i_mA(st->discharge_i_set_mA + 100);
                char buf[20], tmp[12];
                fmt_milli(tmp, s_bridge.get_state()->discharge_i_set_mA);
                snprintf(buf, sizeof(buf), "I: %sA", tmp);
                lv_label_set_text(bms_ui_view_get_button_label(obj), buf);
            }
            else if(obj == bms_ui_view_btn_baud()) {
                s_bridge.set_baud_rate((st->baud_rate_idx + 1) % 2);
                lv_label_set_text(bms_ui_view_get_button_label(obj),
                                 s_bridge.get_state()->baud_rate_idx ? "Baud:115K" : "Baud:9K6");
            }
            else if(obj == bms_ui_view_btn_port()) {
                s_bridge.set_port((st->port_idx + 1) % 2);
                lv_label_set_text(bms_ui_view_get_button_label(obj),
                                 s_bridge.get_state()->port_idx ? "Port:UART1" : "Port:UART0");
            }
        }
        else if(key == LV_KEY_DOWN || key == LV_KEY_LEFT) {
            if(obj == bms_ui_view_btn_uset()) {
                s_bridge.set_charge_u_mV(st->charge_u_set_mV - 10);
                char buf[20], tmp[12];
                fmt_milli(tmp, s_bridge.get_state()->charge_u_set_mV);
                snprintf(buf, sizeof(buf), "U: %sV", tmp);
                lv_label_set_text(bms_ui_view_get_button_label(obj), buf);
            }
            else if(obj == bms_ui_view_btn_iset()) {
                s_bridge.set_charge_i_mA(st->charge_i_set_mA - 50);
                char buf[20], tmp[12];
                fmt_milli(tmp, s_bridge.get_state()->charge_i_set_mA);
                snprintf(buf, sizeof(buf), "I: %sA", tmp);
                lv_label_set_text(bms_ui_view_get_button_label(obj), buf);
            }
            else if(obj == bms_ui_view_btn_idis()) {
                s_bridge.set_discharge_i_mA(st->discharge_i_set_mA - 100);
                char buf[20], tmp[12];
                fmt_milli(tmp, s_bridge.get_state()->discharge_i_set_mA);
                snprintf(buf, sizeof(buf), "I: %sA", tmp);
                lv_label_set_text(bms_ui_view_get_button_label(obj), buf);
            }
            else if(obj == bms_ui_view_btn_baud()) {
                s_bridge.set_baud_rate((st->baud_rate_idx + 1) % 2);
                lv_label_set_text(bms_ui_view_get_button_label(obj),
                                 s_bridge.get_state()->baud_rate_idx ? "Baud:115K" : "Baud:9K6");
            }
            else if(obj == bms_ui_view_btn_port()) {
                s_bridge.set_port((st->port_idx + 1) % 2);
                lv_label_set_text(bms_ui_view_get_button_label(obj),
                                 s_bridge.get_state()->port_idx ? "Port:UART1" : "Port:UART0");
            }
        }
        else if(key == LV_KEY_ESC || key == LV_KEY_ENTER) {
            lv_group_set_editing(g, false);
            lv_obj_remove_state(obj, LV_STATE_USER_1);
            lv_obj_t* bar = bms_ui_view_get_button_bar(obj);
            if(bar) lv_obj_remove_flag(bar, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        /* Navigation mode */
        if(obj == bms_ui_view_footer_menu() && key == LV_KEY_ENTER && !bms_ui_view_footer_selected()) {
            bms_ui_view_set_footer_selected(true);
            lv_obj_add_state(obj, LV_STATE_USER_1);
            lv_group_set_editing(g, true);
            bms_ui_view_update_footer_label(bms_ui_view_current_page(), false);
            return;
        }

        if((key == ' ' || key == LV_KEY_ESC) && !bms_ui_view_footer_selected()) {
            bms_ui_view_switch_page((bms_ui_view_current_page() + 1) % 4);
        }
        else if(bms_ui_view_current_page() == 0) {
            if(key == LV_KEY_RIGHT || key == LV_KEY_DOWN) bms_ui_view_switch_page(1);
            else if(key == LV_KEY_LEFT || key == LV_KEY_UP) {
                bms_ui_view_switch_page(3);
                lv_group_focus_obj(bms_ui_view_footer_menu());
            }
        }
        else if(bms_ui_view_current_page() == 1) {
            if(key == LV_KEY_RIGHT || key == LV_KEY_DOWN) {
                if(obj == bms_ui_view_footer_menu()) bms_ui_view_switch_page(2);
                else lv_group_focus_next(g);
            } else if(key == LV_KEY_LEFT || key == LV_KEY_UP) {
                if(obj == bms_ui_view_btn_uset()) bms_ui_view_switch_page(0);
                else lv_group_focus_prev(g);
            }
        }
        else if(bms_ui_view_current_page() == 2) {
            if(key == LV_KEY_RIGHT || key == LV_KEY_DOWN) {
                if(obj == bms_ui_view_footer_menu()) bms_ui_view_switch_page(3);
                else lv_group_focus_next(g);
            } else if(key == LV_KEY_LEFT || key == LV_KEY_UP) {
                if(obj == bms_ui_view_btn_idis()) {
                    bms_ui_view_switch_page(1);
                    lv_group_focus_obj(bms_ui_view_footer_menu());
                } else lv_group_focus_prev(g);
            }
        }
        else if(bms_ui_view_current_page() == 3) {
            if(key == LV_KEY_RIGHT || key == LV_KEY_DOWN) {
                if(obj == bms_ui_view_footer_menu()) bms_ui_view_switch_page(0);
                else lv_group_focus_next(g);
            } else if(key == LV_KEY_LEFT || key == LV_KEY_UP) {
                if(obj == bms_ui_view_btn_baud()) {
                    bms_ui_view_switch_page(2);
                    lv_group_focus_obj(bms_ui_view_footer_menu());
                } else lv_group_focus_prev(g);
            }
        }
    }
}

/**********************
 *   GLOBAL KEY HANDLER
 **********************/

static void on_global_key(lv_event_t* e)
{
    lv_key_t key = lv_event_get_key(e);
    lv_group_t* g = lv_group_get_default();
    const bms_state_t* st = s_bridge.get_state();

    if(st->charge_active || st->discharge_active) return;

    if((key == ' ' || key == LV_KEY_ESC) && (!g || !lv_group_get_editing(g)) && !bms_ui_view_footer_selected()) {
        bms_ui_view_switch_page((bms_ui_view_current_page() + 1) % 4);
    }
    else if(bms_ui_view_current_page() == 0 && (!g || !lv_group_get_editing(g))) {
        if(key == LV_KEY_RIGHT || key == LV_KEY_DOWN) bms_ui_view_switch_page(1);
        else if(key == LV_KEY_LEFT || key == LV_KEY_UP) bms_ui_view_switch_page(3);
    }
}

/**********************
 *   FOCUS HANDLERS
 **********************/

static void on_button_focus(lv_event_t* e)
{
    lv_obj_t* btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    const bms_state_t* st = s_bridge.get_state();

    if(code == LV_EVENT_FOCUSED) {
        if(st->charge_active && bms_ui_view_btn_charge_toggle() && btn != bms_ui_view_btn_charge_toggle()) {
            lv_group_focus_obj(bms_ui_view_btn_charge_toggle());
            return;
        }
        if(st->discharge_active && bms_ui_view_btn_discharge_toggle() && btn != bms_ui_view_btn_discharge_toggle()) {
            lv_group_focus_obj(bms_ui_view_btn_discharge_toggle());
            return;
        }
    }

    lv_obj_t* bar = bms_ui_view_get_button_bar(btn);
    if(!bar) return;

    bool is_toggle = (btn == bms_ui_view_btn_charge_toggle() || btn == bms_ui_view_btn_discharge_toggle());
    bool is_active = (btn == bms_ui_view_btn_charge_toggle() && st->charge_active) ||
                     (btn == bms_ui_view_btn_discharge_toggle() && st->discharge_active);

    if(is_toggle && is_active) {
        lv_obj_remove_flag(bar, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if(code == LV_EVENT_FOCUSED) {
        if(!lv_obj_has_state(btn, LV_STATE_USER_1)) lv_obj_remove_flag(bar, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(bar, LV_OBJ_FLAG_HIDDEN);
    } else if(code == LV_EVENT_DEFOCUSED) {
        lv_obj_add_flag(bar, LV_OBJ_FLAG_HIDDEN);
    }
}

static void on_footer_focus(lv_event_t* e)
{
    lv_obj_t* btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    const bms_state_t* st = s_bridge.get_state();

    if(code == LV_EVENT_FOCUSED) {
        if(st->charge_active && bms_ui_view_btn_charge_toggle()) {
            lv_group_focus_obj(bms_ui_view_btn_charge_toggle());
            return;
        }
        if(st->discharge_active && bms_ui_view_btn_discharge_toggle()) {
            lv_group_focus_obj(bms_ui_view_btn_discharge_toggle());
            return;
        }
    }

    if(code == LV_EVENT_FOCUSED) {
        if(bms_ui_view_footer_selected()) {
            lv_obj_add_state(btn, LV_STATE_USER_1);
        } else {
            lv_obj_set_style_bg_color(btn, lv_color_make(0x00, 0x66, 0xBB), 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        }
        /* Restore footer label text color to white when focused */
        lv_obj_t* footer_lbl = bms_ui_view_get_button_label(btn);
        if(footer_lbl) lv_obj_set_style_text_color(footer_lbl, lv_color_white(), 0);
    } else if(code == LV_EVENT_DEFOCUSED) {
        lv_obj_remove_state(btn, LV_STATE_USER_1);
        if(!bms_ui_view_switching_page()) {
            bms_ui_view_set_footer_selected(false);
            lv_group_t* g = lv_group_get_default();
            if(g) lv_group_set_editing(g, false);
        }
        if(!bms_ui_view_footer_selected()) {
            lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
            lv_obj_t* footer_lbl = bms_ui_view_get_button_label(btn);
            if(footer_lbl) lv_obj_set_style_text_color(footer_lbl, lv_color_make(0x00, 0xE5, 0xFF), 0);
        }
    }
}
