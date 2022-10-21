/*
 * app_wifi.c
 *
 *  Created on: Apr 27, 2021
 *      Author: huybk
 */


#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
//#include "esp_netif.h"
//#include "esp_system.h"
//#include "esp_wifi.h"
#include <string.h>
#include "app_uart.h"
#include "lwrb.h"
#include "min.h"
#include "min_id.h"
#if USE_APP_CLI
#include "app_cli.h"
#endif

#define PATTERN_CHR_NUM    (3)

static const char *TAG = "app_uart";
QueueHandle_t uart1_queue;
QueueHandle_t uart0_queue;
extern lwrb_t data_uart_module_rb;
static lwrb_t* data_uart_module_rb_ptr = &data_uart_module_rb;

extern min_context_t m_min_context;


void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    size_t buffered_size;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
    // uint8_t* dtmp_uart0 = (uint8_t*) malloc(RD_BUF_SIZE);
    for(;;) {
        // need add more uart for 4g, rs485, and so far
        //Waiting for UART event.
        if(xQueueReceive(uart1_queue, (void * )&event, (TickType_t)500)) {
            bzero(dtmp, RD_BUF_SIZE);
            ESP_LOGI(TAG, "uart[%d] event:", UART_NUM_1);
            switch(event.type) {
                //Event of UART receving data
                /*We'd better handler data event fast, there would be much more data events than
                other types of events. If we take too much time on data event, the queue might
                be full.*/
                case UART_DATA:
                    //ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                    uart_read_bytes(UART_NUM_1, dtmp, event.size, portMAX_DELAY);
                    min_rx_feed(&m_min_context, (uint8_t *)dtmp, event.size);
#if USE_APP_CLI
                    for (size_t i = 0; i < event.size; i++)
                    {
                        app_cli_poll(dtmp[i]);
                    }
#endif
                    //uart_write_bytes(UART_NUM_1, (const char*) dtmp, event.size); //no need echo

                    break;
                //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(UART_NUM_1);
                    xQueueReset(uart1_queue);
                    break;
                //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(UART_NUM_1);
                    xQueueReset(uart1_queue);
                    break;
                //Event of UART RX break detected
                case UART_BREAK:
                    ESP_LOGI(TAG, "uart1 rx break");
                    break;
                //Event of UART parity check error
                case UART_PARITY_ERR:
                    ESP_LOGI(TAG, "uart1 parity error");
                    break;
                //Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGI(TAG, "uart1 frame error");
                    break;
                //UART_PATTERN_DET
                case UART_PATTERN_DET:
                    uart_get_buffered_data_len(UART_NUM_1, &buffered_size);
                    int pos = uart_pattern_pop_pos(UART_NUM_1);
                    ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                    if (pos == -1) {
                        // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
                        // record the position. We should set a larger queue size.
                        // As an example, we directly flush the rx buffer here.
                        uart_flush_input(UART_NUM_1);
                    } else {
                        uart_read_bytes(UART_NUM_1, dtmp, pos, 100 / portTICK_PERIOD_MS);
                        uint8_t pat[PATTERN_CHR_NUM + 1];
                        memset(pat, 0, sizeof(pat));
                        uart_read_bytes(UART_NUM_1, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
                        ESP_LOGI(TAG, "read data: %s", dtmp);
                        ESP_LOGI(TAG, "read pat : %s", pat);
                    }
                    break;
                //Others
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
        }
        // else if (xQueueReceive(uart0_queue, (void * )&event, (TickType_t)500))
        // {
        //     bzero(dtmp_uart0, RD_BUF_SIZE);
        //     ESP_LOGI(TAG, "uart[%d] event:", UART_NUM_0);
        //     switch(event.type) {
        //         //Event of UART receving data
        //         /*We'd better handler data event fast, there would be much more data events than
        //         other types of events. If we take too much time on data event, the queue might
        //         be full.*/
        //         case UART_DATA:
        //             //ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
        //             uart_read_bytes(UART_NUM_0, dtmp_uart0, event.size, portMAX_DELAY);
        //             ESP_LOGI(TAG, "[UART0 DATA EVT]:");
        //             //lwrb_write (data_uart_module_rb_ptr, dtmp_uart0, event.size); //write data to ringbuff
        //             min_rx_feed(&m_min_context, dtmp_uart0, event.size);
        //             //uart_write_bytes(UART_NUM_0, (const char*) dtmp_uart0, event.size);
        //             break;
        //         //Event of HW FIFO overflow detected
        //         case UART_FIFO_OVF:
        //             ESP_LOGI(TAG, "hw fifo overflow");
        //             // If fifo overflow happened, you should consider adding flow control for your application.
        //             // The ISR has already reset the rx FIFO,
        //             // As an example, we directly flush the rx buffer here in order to read more data.
        //             uart_flush_input(UART_NUM_0);
        //             xQueueReset(uart0_queue);
        //             break;
        //         //Event of UART ring buffer full
        //         case UART_BUFFER_FULL:
        //             ESP_LOGI(TAG, "ring buffer full");
        //             // If buffer full happened, you should consider encreasing your buffer size
        //             // As an example, we directly flush the rx buffer here in order to read more data.
        //             uart_flush_input(UART_NUM_0);
        //             xQueueReset(uart0_queue);
        //             break;
        //         //Event of UART RX break detected
        //         case UART_BREAK:
        //             ESP_LOGI(TAG, "uart rx break");
        //             break;
        //         //Event of UART parity check error
        //         case UART_PARITY_ERR:
        //             ESP_LOGI(TAG, "uart parity error");
        //             break;
        //         //Event of UART frame error
        //         case UART_FRAME_ERR:
        //             ESP_LOGI(TAG, "uart frame error");
        //             break;
        //         //UART_PATTERN_DET
        //         case UART_PATTERN_DET:
        //             uart_get_buffered_data_len(UART_NUM_0, &buffered_size);
        //             int pos = uart_pattern_pop_pos(UART_NUM_0);
        //             ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
        //             if (pos == -1) {
        //                 // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
        //                 // record the position. We should set a larger queue size.
        //                 // As an example, we directly flush the rx buffer here.
        //                 uart_flush_input(UART_NUM_0);
        //             } else {
        //                 uart_read_bytes(UART_NUM_0, dtmp, pos, 100 / portTICK_PERIOD_MS);
        //                 uint8_t pat[PATTERN_CHR_NUM + 1];
        //                 memset(pat, 0, sizeof(pat));
        //                 uart_read_bytes(UART_NUM_0, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
        //                 ESP_LOGI(TAG, "read data: %s", dtmp);
        //                 ESP_LOGI(TAG, "read pat : %s", pat);
        //             }
        //             break;
        //         //Others
        //         default:
        //             ESP_LOGI(TAG, "uart event type: %d", event.type);
        //             break;
        //     }
        // }
    }
    free(dtmp);
    free(dtmp_uart0);
    dtmp = NULL;
    vTaskDelete(NULL);
}