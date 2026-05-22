#ifndef BMS_UI_H
#define BMS_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

/**
 * Initialize the BMS simulator UI.
 */
void bms_ui_init(void);

/**
 * Update the global predicted SoC value.
 * @param soc_val The predicted SoC value from the host (0 to 100).
 */
void bms_ui_update_soc(uint8_t soc_val);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*BMS_UI_H*/
