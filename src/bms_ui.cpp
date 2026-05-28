#include "bms_ui.h"

extern "C" {
#include "bms_ui_view.h"
#include "bms_ui_ctrl.h"

#ifdef BMS_SIM
#include "bms_sim.h"
#define BMS_INIT        bms_sim_init
#define BMS_TICK_200    bms_sim_tick_200ms
#define BMS_TICK_1000   bms_sim_tick_1000ms
#define BMS_GET_STATE   bms_sim_get_state
#define BMS_SET_CHG_EN  bms_sim_set_charge_enable
#define BMS_SET_DSC_EN  bms_sim_set_discharge_enable
#define BMS_SET_CHG_U   bms_sim_set_charge_u_mV
#define BMS_SET_CHG_I   bms_sim_set_charge_i_mA
#define BMS_SET_DSC_I   bms_sim_set_discharge_i_mA
#define BMS_SET_BAUD    bms_sim_set_baud_rate
#define BMS_SET_PORT    bms_sim_set_port
#else
#include "bms_hw.h"
#define BMS_INIT        bms_hw_init
#define BMS_TICK_200    bms_hw_tick_200ms
#define BMS_TICK_1000   bms_hw_tick_1000ms
#define BMS_GET_STATE   bms_hw_get_state
#define BMS_SET_CHG_EN  bms_hw_set_charge_enable
#define BMS_SET_DSC_EN  bms_hw_set_discharge_enable
#define BMS_SET_CHG_U   bms_hw_set_charge_u_mV
#define BMS_SET_CHG_I   bms_hw_set_charge_i_mA
#define BMS_SET_DSC_I   bms_hw_set_discharge_i_mA
#define BMS_SET_BAUD    bms_hw_set_baud_rate
#define BMS_SET_PORT    bms_hw_set_port
#endif
}

static void tick_200_cb(lv_timer_t*) {
    BMS_TICK_200();
    bms_ui_view_refresh(BMS_GET_STATE());
}

static void tick_1000_cb(lv_timer_t*) {
    BMS_TICK_1000();
    bms_ui_view_refresh(BMS_GET_STATE());
}

extern "C" void bms_ui_init(void)
{
    BMS_INIT();
    bms_ui_view_init();

    bms_data_bridge_t bridge;
    bridge.get_state         = BMS_GET_STATE;
    bridge.set_charge_enable = BMS_SET_CHG_EN;
    bridge.set_discharge_enable = BMS_SET_DSC_EN;
    bridge.set_charge_u_mV   = BMS_SET_CHG_U;
    bridge.set_charge_i_mA   = BMS_SET_CHG_I;
    bridge.set_discharge_i_mA = BMS_SET_DSC_I;
    bridge.set_baud_rate     = BMS_SET_BAUD;
    bridge.set_port          = BMS_SET_PORT;

    bms_ui_ctrl_init(&bridge);

    lv_timer_create(tick_200_cb, 200, nullptr);
    lv_timer_create(tick_1000_cb, 1000, nullptr);
}

extern "C" void bms_ui_update_soc(uint8_t soc_val)
{
    bms_ui_view_update_soc(soc_val);
}
