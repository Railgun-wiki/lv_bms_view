#ifndef BMS_UI_INTERNAL_H
#define BMS_UI_INTERNAL_H

#include "lvgl.h"
#include "bms_state.h"

/* All widget pointers shared between view submodules */
typedef struct {
    /* Page containers */
    lv_obj_t* pages[4];

    /* Header */
    lv_obj_t* lblMode;
    lv_obj_t* lblSoc;
    lv_obj_t* barSoc;
    lv_obj_t* barSocInd;
    lv_obj_t* lblLink;

    /* Footer */
    lv_obj_t* lblFooter;
    lv_obj_t* btnFooter;
    lv_obj_t* circle;

    /* Page 0: SoC */
    lv_obj_t* lblSocBig;
    lv_obj_t* lblTrend;
    lv_obj_t* lblU, *lblI, *lblT, *lblR;

    /* Page 1: CCCV */
    lv_obj_t* btnUset, *btnIset, *btnChgToggle;
    lv_obj_t* chartP2;
    lv_chart_series_t *p2SerU, *p2SerI;
    lv_obj_t* lblP2Readout;

    /* Page 2: Discharge */
    lv_obj_t* btnIdis, *btnDscToggle;
    lv_obj_t* chartP3;
    lv_chart_series_t *p3SerU, *p3SerI;
    lv_obj_t* lblP3Readout;

    /* Page 3: System */
    lv_obj_t* btnBaud, *btnPort;
    lv_obj_t* lblTerminal;
} bms_ui_widgets_t;

/* Utility */
void bms_ui_strip_decorations(lv_obj_t* obj);
lv_obj_t* bms_ui_get_button_label(lv_obj_t* btn);
lv_obj_t* bms_ui_get_button_bar(lv_obj_t* btn);

/* Refresh (implemented in bms_ui_refresh.c) */
void bms_ui_view_refresh_impl(bms_ui_widgets_t* w, const bms_state_t* s, int currentPage);
void bms_ui_view_update_soc_impl(bms_ui_widgets_t* w, uint8_t soc);
void bms_ui_view_update_toggle_style(lv_obj_t* btn, bool active);

#endif /* BMS_UI_INTERNAL_H */
