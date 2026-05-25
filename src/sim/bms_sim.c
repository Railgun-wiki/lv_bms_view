#include "bms_sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bms_state_t s_state;

/* Floating-point physics (PC simulation only) */
static float s_soc = 85.0f;
static float s_ocv = 4.12f;
static float s_u   = 4.12f;
static float s_i   = 0.00f;
static float s_t   = 25.0f;
static float s_r   = 0.05f;

static void sync_to_state(void)
{
    s_state.soc_x10         = (int32_t)(s_soc * 10.0f);
    s_state.voltage_mV      = (int32_t)(s_u * 1000.0f);
    s_state.current_mA      = (int32_t)(s_i * 1000.0f);
    s_state.temperature_x10 = (int32_t)(s_t * 10.0f);
    s_state.resistance_mOhm = (int32_t)(s_r * 1000.0f);
}

void bms_sim_init(void)
{
    memset(&s_state, 0, sizeof(s_state));
    s_state.soc_x10            = 850;
    s_state.voltage_mV         = 4120;
    s_state.current_mA         = 0;
    s_state.temperature_x10    = 250;
    s_state.resistance_mOhm    = 50;
    s_state.charge_u_set_mV    = 4200;
    s_state.charge_i_set_mA    = 2000;
    s_state.discharge_i_set_mA = 3000;
    s_state.baud_rate_idx      = 1;
    s_state.predicted_soc      = 85;
    s_state.host_online        = true;
    s_state.rx_packet_count    = 104;

    snprintf(s_state.log_lines[0], 64, "SYNC -> ACTIVE");
    snprintf(s_state.log_lines[1], 64, "CHIRAL LINK ESTABLISHED");
    snprintf(s_state.log_lines[2], 64, "SYS TELEMETRY STATUS: OK");
    snprintf(s_state.log_lines[3], 64, "");
}

void bms_sim_tick_200ms(void)
{
    float norm_soc = s_soc / 100.0f;
    s_ocv = 3.0f + 1.15f * norm_soc
          - 0.08f * (1.0f - norm_soc) * (1.0f - norm_soc) * (1.0f - norm_soc);

    if(s_state.charge_active) {
        float target_u = s_state.charge_u_set_mV / 1000.0f;
        float target_i = s_state.charge_i_set_mA / 1000.0f;

        if(s_u < target_u) {
            s_i = target_i;
            s_u = s_ocv + s_i * s_r;
            if(s_u >= target_u) s_u = target_u;
        } else {
            s_u = target_u;
            s_i = (target_u - s_ocv) / s_r;
            if(s_i < 0.05f) s_i = 0.0f;
        }

        s_soc += (s_i * 0.2f / 3600.0f) / 2.6f * 100.0f;
        if(s_soc > 100.0f) s_soc = 100.0f;
        s_t += (s_i * s_i * s_r * 0.2f * 0.05f) - (s_t - 25.0f) * 0.005f;
    }
    else if(s_state.discharge_active) {
        float target_i = s_state.discharge_i_set_mA / 1000.0f;
        s_i = -target_i;
        s_u = s_ocv + s_i * s_r;

        s_soc += (s_i * 0.2f / 3600.0f) / 2.6f * 100.0f;
        if(s_soc < 0.0f) {
            s_soc = 0.0f;
            s_state.discharge_active = false;
            s_state.discharge_auto_stopped = true;
        }
        if(s_u < 2.80f) {
            s_state.discharge_active = false;
            s_state.low_volt_alert = true;
            s_state.discharge_auto_stopped = true;
            s_i = 0.00f;
            s_u = s_ocv;
        }
        s_t += (s_i * s_i * s_r * 0.2f * 0.05f) - (s_t - 25.0f) * 0.005f;
    }
    else {
        s_i = 0.00f;
        s_u = s_ocv;
        s_t -= (s_t - 25.0f) * 0.005f;
    }

    sync_to_state();
}

void bms_sim_tick_1000ms(void)
{
    s_state.rx_packet_count += (rand() % 2 + 1);

    if(rand() % 12 == 0) {
        s_state.host_online = !s_state.host_online;
    }

    float difference = s_soc - (float)s_state.predicted_soc;
    if(difference > 1.0f) {
        s_state.predicted_soc++;
    } else if(difference < -1.0f) {
        s_state.predicted_soc--;
    } else {
        if(rand() % 10 == 0) {
            s_state.predicted_soc = (uint8_t)s_soc;
        }
    }
    if(s_state.predicted_soc > 100) s_state.predicted_soc = 100;

    static int log_cycle = 0;
    log_cycle++;

    char log_entry[64];
    if(!s_state.host_online) {
        switch(log_cycle % 3) {
            case 0: snprintf(log_entry, 64, "LINK -> OFFLINE"); break;
            case 1: snprintf(log_entry, 64, "RX -> TIMEOUT ERROR"); break;
            case 2: snprintf(log_entry, 64, "SYS -> CHIRAL DISCONN"); break;
        }
    } else {
        switch(log_cycle % 4) {
            case 0: snprintf(log_entry, 64, "RX -> SOC: %03d%%", s_state.predicted_soc); break;
            case 1: snprintf(log_entry, 64, "RX -> PACKETS: %04d", s_state.rx_packet_count); break;
            case 2: snprintf(log_entry, 64, "RX -> CELL_U: %ld.%02ldV",
                             (long)(s_state.voltage_mV / 1000),
                             (long)((s_state.voltage_mV < 0 ? -s_state.voltage_mV : s_state.voltage_mV) % 1000 / 10)); break;
            case 3: snprintf(log_entry, 64, "LINK -> SYN %s", s_state.baud_rate_idx ? "115K" : "9K6"); break;
        }
    }

    strncpy(s_state.log_lines[3], s_state.log_lines[2], 64);
    strncpy(s_state.log_lines[2], s_state.log_lines[1], 64);
    strncpy(s_state.log_lines[1], s_state.log_lines[0], 64);
    strncpy(s_state.log_lines[0], log_entry, 64);
}

const bms_state_t* bms_sim_get_state(void) { return &s_state; }

void bms_sim_set_charge_enable(bool enable)
{
    s_state.charge_active = enable;
    s_state.discharge_active = false;
    s_state.low_volt_alert = false;
    s_state.charge_auto_stopped = false;
    s_state.discharge_auto_stopped = false;
}

void bms_sim_set_discharge_enable(bool enable)
{
    s_state.discharge_active = enable;
    s_state.charge_active = false;
    s_state.low_volt_alert = false;
    s_state.charge_auto_stopped = false;
    s_state.discharge_auto_stopped = false;
}

void bms_sim_set_charge_u_mV(int32_t mV)
{
    s_state.charge_u_set_mV = mV;
    if(s_state.charge_u_set_mV > 5000) s_state.charge_u_set_mV = 5000;
    if(s_state.charge_u_set_mV < 0) s_state.charge_u_set_mV = 0;
}

void bms_sim_set_charge_i_mA(int32_t mA)
{
    s_state.charge_i_set_mA = mA;
    if(s_state.charge_i_set_mA > 5000) s_state.charge_i_set_mA = 5000;
    if(s_state.charge_i_set_mA < 0) s_state.charge_i_set_mA = 0;
}

void bms_sim_set_discharge_i_mA(int32_t mA)
{
    s_state.discharge_i_set_mA = mA;
    if(s_state.discharge_i_set_mA > 10000) s_state.discharge_i_set_mA = 10000;
    if(s_state.discharge_i_set_mA < 0) s_state.discharge_i_set_mA = 0;
}

void bms_sim_set_baud_rate(int idx) { s_state.baud_rate_idx = idx % 2; }
void bms_sim_set_port(int idx)      { s_state.port_idx = idx % 2; }
