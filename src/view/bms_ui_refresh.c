#include "bms_ui_internal.h"
#include "bms_ui_styles.h"
#include "bms_state.h"
#include <stdio.h>

/**********************
 *  INTEGER FORMATTING
 **********************/

static void fmt_milli(char* buf, int32_t val)
{
    int32_t whole = val / 1000;
    int32_t frac = (val < 0 ? -val : val) % 1000 / 10;
    snprintf(buf, 12, "%ld.%02ld", (long)whole, (long)frac);
}

static void fmt_x10(char* buf, int32_t val)
{
    int32_t whole = val / 10;
    int32_t frac = (val < 0 ? -val : val) % 10;
    snprintf(buf, 8, "%ld.%ld", (long)whole, (long)frac);
}

static void format_readout(char* buf, size_t len, int32_t mV, int32_t mA)
{
    char tu[12], ti[12], tp[12];
    int32_t power_mW = (mV / 100) * (mA / 100) / 100;
    fmt_milli(tu, mV);
    fmt_milli(ti, mA);
    fmt_milli(tp, power_mW);
    snprintf(buf, len, "U:%sV I:%sA P:%sW", tu, ti, tp);
}

/**********************
 *  STYLE UPDATES
 **********************/

void bms_ui_view_update_toggle_style(lv_obj_t* btn, bool active)
{
    if(!btn) return;
    lv_obj_t* bar = bms_ui_get_button_bar(btn);

    lv_color_t bg = active ? COLOR_BG : COLOR_CYAN;
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_bg_color(btn, COLOR_BG, LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(btn, COLOR_BG, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_color(btn, COLOR_BG, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_FOCUSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);

    if(bar) {
        if(active) lv_obj_remove_flag(bar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_t* s1 = lv_obj_get_child(bar, 0);
        lv_obj_t* s2 = lv_obj_get_child(bar, 1);
        lv_obj_t* s3 = lv_obj_get_child(bar, 2);
        lv_obj_t* s4 = lv_obj_get_child(bar, 3);
        if(active) {
            if(s1) lv_obj_set_style_bg_color(s1, COLOR_GOLD, 0);
            if(s2) lv_obj_set_style_bg_color(s2, lv_color_make(0xC0, 0x77, 0x00), 0);
            if(s3) lv_obj_set_style_bg_color(s3, lv_color_make(0x80, 0x50, 0x00), 0);
            if(s4) lv_obj_set_style_bg_color(s4, lv_color_make(0x40, 0x27, 0x00), 0);
        } else {
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

static void update_link_status(lv_obj_t* lbl, bool online)
{
    if(!lbl) return;
    if(online) {
        lv_label_set_text(lbl, "ONLINE");
        lv_obj_set_style_text_color(lbl, COLOR_DS_TEAL, 0);
    } else {
        lv_label_set_text(lbl, "OFFLINE");
        lv_obj_set_style_text_color(lbl, COLOR_RED, 0);
    }
}

/**********************
 *  REFRESH
 **********************/

void bms_ui_view_update_soc_impl(bms_ui_widgets_t* w, uint8_t soc)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", soc);
    if(w->lblSoc) { lv_label_set_text(w->lblSoc, buf); lv_obj_center(w->lblSoc); }
    if(w->barSocInd) lv_obj_set_width(w->barSocInd, (int32_t)soc * 58 / 100);
}

void bms_ui_view_refresh_impl(bms_ui_widgets_t* w, const bms_state_t* s, int currentPage)
{
    if(!s || !w) return;

    if(currentPage == 0) {
        char buf[24], tmp[12];

        fmt_milli(tmp, s->voltage_mV);
        snprintf(buf, sizeof(buf), "U: %s V", tmp);
        lv_label_set_text(w->lblU, buf);

        fmt_milli(tmp, s->current_mA);
        snprintf(buf, sizeof(buf), "I: %s A", tmp);
        lv_label_set_text(w->lblI, buf);

        fmt_x10(tmp, s->temperature_x10);
        snprintf(buf, sizeof(buf), "T: %s C", tmp);
        lv_label_set_text(w->lblT, buf);

        if(s->charge_active) {
            lv_label_set_text(w->lblTrend, "[+ CHARGING]");
            lv_obj_set_style_text_color(w->lblTrend, COLOR_GOLD, 0);
        } else if(s->discharge_active) {
            lv_label_set_text(w->lblTrend, "[- DISCHG]");
            lv_obj_set_style_text_color(w->lblTrend, COLOR_RED, 0);
        } else if(s->low_volt_alert) {
            lv_label_set_text(w->lblTrend, "[! L-VOLT]");
            lv_obj_set_style_text_color(w->lblTrend, COLOR_RED, 0);
        } else {
            lv_label_set_text(w->lblTrend, "[STANDBY]");
            lv_obj_set_style_text_color(w->lblTrend, COLOR_GRAY, 0);
        }
    }

    if(currentPage == 1) {
        char buf[40];
        int32_t mA = s->charge_active ? s->current_mA : 0;
        format_readout(buf, sizeof(buf), s->voltage_mV, mA);
        lv_label_set_text(w->lblP2Readout, buf);
        lv_chart_set_next_value(w->chartP2, w->p2SerU, s->voltage_mV);
        lv_chart_set_next_value(w->chartP2, w->p2SerI, s->charge_active ? s->current_mA : 0);
    }

    if(currentPage == 2) {
        char buf[36];
        if(s->low_volt_alert) {
            snprintf(buf, sizeof(buf), "CUTOFF! U < 2.80V");
            lv_label_set_text(w->lblP3Readout, buf);
            lv_obj_set_style_text_color(w->lblP3Readout, COLOR_RED, 0);
        } else {
            int32_t mA = s->discharge_active ? (-s->current_mA) : 0;
            format_readout(buf, sizeof(buf), s->voltage_mV, mA);
            lv_label_set_text(w->lblP3Readout, buf);
            lv_obj_set_style_text_color(w->lblP3Readout, COLOR_GRAY, 0);
        }
        lv_chart_set_next_value(w->chartP3, w->p3SerU, s->voltage_mV);
        lv_chart_set_next_value(w->chartP3, w->p3SerI, s->discharge_active ? (-s->current_mA / 2) : 0);
    }

    if(currentPage == 3) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s\n%s\n%s\n%s",
                 s->log_lines[0], s->log_lines[1], s->log_lines[2], s->log_lines[3]);
        lv_label_set_text(w->lblTerminal, buf);
    }

    /* Auto-stopped toggle updates */
    if(s->charge_auto_stopped && w->btnChgToggle) {
        lv_label_set_text(bms_ui_get_button_label(w->btnChgToggle), "CHG: OFF");
        bms_ui_view_update_toggle_style(w->btnChgToggle, false);
    }
    if(s->discharge_auto_stopped && w->btnDscToggle) {
        lv_label_set_text(bms_ui_get_button_label(w->btnDscToggle), "DSC: OFF");
        bms_ui_view_update_toggle_style(w->btnDscToggle, false);
    }

    update_link_status(w->lblLink, s->host_online);

    /* Footer circle */
    if(w->circle) {
        if(currentPage == 0 || currentPage == 3) {
            lv_obj_set_style_bg_color(w->circle, COLOR_GOLD, 0);
            lv_obj_set_style_bg_opa(w->circle, LV_OPA_COVER, 0);
        } else {
            bool blink_on = (lv_tick_get() % 1000) < 500;
            if(blink_on) {
                bool active = (currentPage == 1) ? s->charge_active : s->discharge_active;
                lv_obj_set_style_bg_color(w->circle, active ? COLOR_RED : COLOR_DS_TEAL, 0);
                lv_obj_set_style_bg_opa(w->circle, LV_OPA_COVER, 0);
            } else {
                lv_obj_set_style_bg_opa(w->circle, LV_OPA_TRANSP, 0);
            }
        }
    }
}
