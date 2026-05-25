#include "bms_ui.h"

extern "C" {
#include "bms_sim.h"
#include "bms_ui_view.h"
#include "bms_ui_ctrl.h"
}

static void sim_tick_cb(lv_timer_t*) {
    bms_sim_tick_200ms();
    bms_ui_view_refresh(bms_sim_get_state());
}

static void host_tick_cb(lv_timer_t*) {
    bms_sim_tick_1000ms();
    bms_ui_view_refresh(bms_sim_get_state());
}

extern "C" void bms_ui_init(void)
{
    bms_sim_init();
    bms_ui_view_init();

    bms_data_bridge_t bridge;
    bridge.get_state         = bms_sim_get_state;
    bridge.set_charge_enable = bms_sim_set_charge_enable;
    bridge.set_discharge_enable = bms_sim_set_discharge_enable;
    bridge.set_charge_u_mV   = bms_sim_set_charge_u_mV;
    bridge.set_charge_i_mA   = bms_sim_set_charge_i_mA;
    bridge.set_discharge_i_mA = bms_sim_set_discharge_i_mA;
    bridge.set_baud_rate     = bms_sim_set_baud_rate;
    bridge.set_port          = bms_sim_set_port;

    bms_ui_ctrl_init(&bridge);

    lv_timer_create(sim_tick_cb, 200, nullptr);
    lv_timer_create(host_tick_cb, 1000, nullptr);
}

extern "C" void bms_ui_update_soc(uint8_t soc_val)
{
    bms_ui_view_update_soc(soc_val);
}
