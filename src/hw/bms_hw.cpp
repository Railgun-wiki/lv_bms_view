extern "C" {
#include "bms_hw.h"
}

#include "ina226.hpp"
#include "dac8562.hpp"
#include <cstring>

static bms_state_t s_state;
static Ina226* s_ina;
static Dac8562* s_dac;

extern "C" void bms_hw_bind(Ina226* ina, Dac8562* dac)
{
    s_ina = ina;
    s_dac = dac;
}

extern "C" void bms_hw_init(void)
{
    memset(&s_state, 0, sizeof(s_state));
}

extern "C" void bms_hw_tick_200ms(void)
{
    s_state.voltage_mV = s_ina->getBusVoltage_mV();
    s_state.current_mA = s_ina->getCurrent_mA();
    /* TODO: NTC ADC temperature reading */
}

extern "C" void bms_hw_tick_1000ms(void)
{
    /* TODO: poll UART host packets, update log_lines */
}

extern "C" const bms_state_t* bms_hw_get_state(void)
{
    return &s_state;
}

extern "C" void bms_hw_set_charge_enable(bool enable)
{
    s_state.charge_active = enable;
    if (!enable) {
        s_dac->writeValue(Dac8562::Channel::DAC_A, 0);
    }
}

extern "C" void bms_hw_set_discharge_enable(bool enable)
{
    s_state.discharge_active = enable;
    if (!enable) {
        s_dac->writeValue(Dac8562::Channel::DAC_B, 0);
    }
}

extern "C" void bms_hw_set_charge_u_mV(int32_t mV)
{
    s_state.charge_u_set_mV = mV;
}

extern "C" void bms_hw_set_charge_i_mA(int32_t mA)
{
    s_state.charge_i_set_mA = mA;
}

extern "C" void bms_hw_set_discharge_i_mA(int32_t mA)
{
    s_state.discharge_i_set_mA = mA;
}

extern "C" void bms_hw_set_baud_rate(int idx)
{
    s_state.baud_rate_idx = idx;
}

extern "C" void bms_hw_set_port(int idx)
{
    s_state.port_idx = idx;
}
