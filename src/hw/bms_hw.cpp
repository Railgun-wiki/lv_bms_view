extern "C" {
#include "bms_hw.h"
}

#include <cstring>

static bms_state_t s_state;

void bms_hw_init(void)
{
    memset(&s_state, 0, sizeof(s_state));
    /* TODO: init INA226 I2C, DAC8562 SPI, NTC ADC, UART */
}

void bms_hw_tick_200ms(void)
{
    /* TODO: read INA226 voltage/current, NTC temperature */
}

void bms_hw_tick_1000ms(void)
{
    /* TODO: poll UART host packets, update log_lines */
}

const bms_state_t* bms_hw_get_state(void)
{
    return &s_state;
}

void bms_hw_set_charge_enable(bool enable)
{
    /* TODO: control charge MOSFET via DAC8562 */
    s_state.charge_active = enable;
}

void bms_hw_set_discharge_enable(bool enable)
{
    /* TODO: control discharge MOSFET */
    s_state.discharge_active = enable;
}

void bms_hw_set_charge_u_mV(int32_t mV)
{
    s_state.charge_u_set_mV = mV;
}

void bms_hw_set_charge_i_mA(int32_t mA)
{
    s_state.charge_i_set_mA = mA;
}

void bms_hw_set_discharge_i_mA(int32_t mA)
{
    s_state.discharge_i_set_mA = mA;
}

void bms_hw_set_baud_rate(int idx)
{
    s_state.baud_rate_idx = idx;
}

void bms_hw_set_port(int idx)
{
    s_state.port_idx = idx;
}
