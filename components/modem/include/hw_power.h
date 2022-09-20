#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define GPIO_GSM_RESET 18
#define GPIO_GSM_PWRKEY 21
#define GPIO_GSM_EN 5
#define GPIO_INPUT_PWRKEY 34
#define GPIO_INPUT_PWRCTRL 35

#define GSM_OUTPUT_SEL (1ULL<<GPIO_GSM_RESET) |(1ULL<<GPIO_GSM_PWRKEY) | (1ULL<<GPIO_GSM_EN)
#define GSM_INPUT_SEL (1ULL<<GPIO_INPUT_PWRKEY) |(1ULL<<GPIO_INPUT_PWRCTRL) //| (1ULL<<GPIO_GSM_EN)
void hw_power_on (void);
void reset_GSM (void);
#ifdef __cplusplus
}
#endif