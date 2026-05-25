#ifndef BMS_STATE_H
#define BMS_STATE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    /* Battery physical state — aligned with INA226 integer API */
    int32_t  soc_x10;            /* SoC * 10 (85.0% = 850) */
    int32_t  voltage_mV;         /* bus voltage mV */
    int32_t  current_mA;         /* current mA (negative = discharge) */
    int32_t  temperature_x10;    /* temperature * 10 (25.0C = 250) */
    int32_t  resistance_mOhm;    /* internal resistance mOhm */

    /* Charge settings */
    int32_t  charge_u_set_mV;    /* target voltage mV */
    int32_t  charge_i_set_mA;    /* target current mA */
    bool     charge_active;

    /* Discharge settings */
    int32_t  discharge_i_set_mA; /* target current mA */
    bool     discharge_active;
    bool     low_volt_alert;

    /* System settings */
    int      baud_rate_idx;      /* 0:9600, 1:115200 */
    int      port_idx;           /* 0:UART0, 1:UART1 */

    /* Host communication */
    uint8_t  predicted_soc;
    bool     host_online;
    uint32_t rx_packet_count;

    /* Status flags */
    bool     charge_auto_stopped;
    bool     discharge_auto_stopped;

    /* Terminal log lines (newest first, 4 lines) */
    char     log_lines[4][64];
} bms_state_t;

#endif /* BMS_STATE_H */
