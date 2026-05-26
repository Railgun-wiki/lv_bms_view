#ifndef BMS_HW_H
#define BMS_HW_H

#include "bms_state.h"

void bms_hw_init(void);
void bms_hw_tick_200ms(void);
void bms_hw_tick_1000ms(void);
const bms_state_t* bms_hw_get_state(void);

void bms_hw_set_charge_enable(bool enable);
void bms_hw_set_discharge_enable(bool enable);
void bms_hw_set_charge_u_mV(int32_t mV);
void bms_hw_set_charge_i_mA(int32_t mA);
void bms_hw_set_discharge_i_mA(int32_t mA);
void bms_hw_set_baud_rate(int idx);
void bms_hw_set_port(int idx);

#endif /* BMS_HW_H */
