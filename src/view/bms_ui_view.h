#ifndef BMS_UI_VIEW_H
#define BMS_UI_VIEW_H

#include "bms_state.h"
#include "lvgl.h"

void bms_ui_view_init(void);
void bms_ui_view_refresh(const bms_state_t* state);
void bms_ui_view_update_soc(uint8_t soc);
int  bms_ui_view_current_page(void);
void bms_ui_view_switch_page(int page_idx);

/* Footer state */
bool bms_ui_view_footer_selected(void);
void bms_ui_view_set_footer_selected(bool v);
bool bms_ui_view_switching_page(void);

/* Widget getters for Controller */
lv_obj_t* bms_ui_view_btn_charge_toggle(void);
lv_obj_t* bms_ui_view_btn_discharge_toggle(void);
lv_obj_t* bms_ui_view_footer_menu(void);
lv_obj_t* bms_ui_view_btn_uset(void);
lv_obj_t* bms_ui_view_btn_iset(void);
lv_obj_t* bms_ui_view_btn_idis(void);
lv_obj_t* bms_ui_view_btn_baud(void);
lv_obj_t* bms_ui_view_btn_port(void);

/* Utility */
lv_obj_t* bms_ui_view_get_button_label(lv_obj_t* btn);
lv_obj_t* bms_ui_view_get_button_bar(lv_obj_t* btn);
void bms_ui_view_update_toggle_style(lv_obj_t* btn, bool active);
void bms_ui_view_update_footer_label(int page, bool bracketed);

#endif /* BMS_UI_VIEW_H */
