#include <stdio.h>
#include "hw_power.h"
#include "driver/gpio.h"
#include "gsm_hardware.h"


void hw_power_on (void)
{
    static uint8_t step = 0;
    switch (step)
    {
        case 0:
            printf ("begin GSM power on sequence\r\n");
            gpio_set_level (GPIO_GSM_EN, 0);
            gpio_set_level (GPIO_GSM_RESET, 1); //RESET
            gpio_set_level (GPIO_GSM_PWRKEY, 0);
            step++;
            break;
        case 1:
            gpio_set_level (GPIO_GSM_RESET, 0); //RESET RELEASE
            gpio_set_level (GPIO_GSM_EN, 1);
            step++;
            printf ("GSM power on\r\n");
            break;
        case 2:
            step ++;
            break;        
        case 3:
            printf("Pulse power key\r\n");
            /* Generate pulse from (1-0-1) |_| to Power On module */
            gpio_set_level (GPIO_GSM_PWRKEY, 1);
            step++;            
            break;
        case 4:
            gpio_set_level (GPIO_GSM_PWRKEY, 0);
            gpio_set_level (GPIO_GSM_RESET, 0);
            step++;
            break;
        case 5: /// polling every 1 s => wait 2 second
        case 6:
            step ++;
            break;
        case 7:
            step ++;
            printf ("POWER ON DONE\r\n");
            break;                     
        default:
            step = 0;
        break;
        
    }
}
void reset_GSM (void)
{
    gpio_set_level (GPIO_GSM_EN, 0);
    gpio_set_level (GPIO_GSM_RESET, 1); //RESET
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    gpio_set_level (GPIO_GSM_EN, 1);
    gpio_set_level (GPIO_GSM_RESET, 0); //RESET RELEASE
    vTaskDelay(1000 / portTICK_PERIOD_MS); //Waiting for Vbat stable 500ms
    /* Turn On module by PowerKey */
    gpio_set_level (GPIO_GSM_PWRKEY, 1);
    vTaskDelay(1500 / portTICK_PERIOD_MS);
    gpio_set_level (GPIO_GSM_PWRKEY, 0);
}
/*
void gsm_hw_ctrl_power_key(bool on)
{
    gpio_set_level(CONFIG_CONFIG_GSM_POWER_KEY_PIN, on ? 1 : 0);
}

void gsm_hw_ctrl_power_en(bool on)
{
//	ESP_LOGI(TAG, "Control pin %u-level %d\r\n", CONFIG_CONFIG_GSM_POWER_EN_PIN, on ? 0 : 1);
	gpio_set_level(CONFIG_CONFIG_GSM_POWER_EN_PIN, on ? 1 : 0);
}
*/