#ifndef BMS_UI_CTRL_H
#define BMS_UI_CTRL_H

#include "bms_state.h"

typedef struct {
    const bms_state_t* (*get_state)(void);
    void (*set_charge_enable)(bool);
    void (*set_discharge_enable)(bool);
    void (*set_charge_u_mV)(int32_t);
    void (*set_charge_i_mA)(int32_t);
    void (*set_discharge_i_mA)(int32_t);
    void (*set_baud_rate)(int);
    void (*set_port)(int);
} bms_data_bridge_t;

void bms_ui_ctrl_init(const bms_data_bridge_t* bridge);

#endif /* BMS_UI_CTRL_H */
