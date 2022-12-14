#include "gsm_hardware.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "min.h"
#include "min_id.h"

#define TAG "gsm_hw"

extern min_context_t m_min_context;
extern void send_min_data(min_msg_t *min_msg);


/*
static IRAM_ATTR void ri_isr_callback(void *arg)
{
    static uint32_t m_high_to_low_timestamp = 0;
    uint32_t current_ms = xTaskGetTickCountFromISR();

	if (gpio_get_level(CONFIG_CONFIG_GSM_RI_PIN) == 0)
	{
		//High to low
		m_high_to_low_timestamp = current_ms;
	}
	else if (m_high_to_low_timestamp) // Low to high
	{
	   uint32_t diff = current_ms - m_high_to_low_timestamp;

	   if (diff > 90 && diff < 150)        // When new sms received, RI pin pull in 120ms (according datasheet)
	   {
		   ets_printf("New sms indicator\r\n");
	   }

	   m_high_to_low_timestamp = 0;
	}
}
*/

void gsm_hardware_initialize(void)
{
	ESP_LOGI(TAG, "GSM IO initialize, power key %d\r\n",
					CONFIG_CONFIG_GSM_POWER_KEY_PIN);
    gpio_config_t io_conf;

/*
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL<<CONFIG_CONFIG_GSM_STATUS_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL<<CONFIG_CONFIG_GSM_RI_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);
    */
    

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL<<CONFIG_GSM_POWER_KEY_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = 0;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(CONFIG_GSM_POWER_KEY_PIN, 1);
    ESP_LOGI(TAG, "GSM io init done");
}

void gsm_hw_ctrl_power_key(bool on)
{
    gpio_set_level(CONFIG_GSM_POWER_KEY_PIN, on ? 1 : 0);
}

void gsm_hw_ctrl_power_en(bool on)
{

    uint8_t logic_level = on;
    static min_msg_t logic_min_msg;
    logic_min_msg.id = MIN_ID_GPIO_CONTROL;
    logic_min_msg.payload = &logic_level;
    logic_min_msg.len = sizeof (uint8_t);
    send_min_data (&logic_min_msg);
}

