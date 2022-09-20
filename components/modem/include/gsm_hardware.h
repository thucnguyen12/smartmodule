#ifndef GSM_HARDWARE_H
#define GSM_HARDWARE_H

#include <stdint.h>
#include <stdbool.h>

#define CONFIG_CONFIG_GSM_POWER_EN_PIN 32
#define CONFIG_CONFIG_GSM_POWER_KEY_PIN 5
//#define CONFIG_CONFIG_GSM_RI_PIN 34
//#define CONFIG_CONFIG_GSM_STATUS_PIN 36
#define CONFIG_CONFIG_GSM_RESET_PIN 4


/**
 * @brief           Initialize gsm gpio
 */
void gsm_hardware_initialize(void);

/**
 * @brief           Control power key
 * @param[in]       on TRUE GSM power key on
 *                     FALSE GSM power key off
 */
void gsm_hw_ctrl_power_key(bool on);

/**
 * @brief           Control power en
 * @param[in]       on TRUE GSM power en on
 *                     FALSE GSM power en off
 */
void gsm_hw_ctrl_power_en(bool on);

#endif /* GSM_HARDWARE_H */
