/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <string.h>
#include "ec600s.h"
#include "esp_log.h"
#include "esp_modem_dce_service.h"
#include "esp_modem_compat.h"
#include "sim800.h"
#include "hw_power.h"
#include "esp_netif.h"
#include "freertos/semphr.h"
#include "esp_modem_netif.h"
#include "hw_power.h"
#define MODEM_RESULT_CODE_POWERDOWN "POWER DOWN"

extern SemaphoreHandle_t GSM_Sem;
extern SemaphoreHandle_t GSM_Task_tearout;
static const char *DCE_TAG = "ec600s";
esp_netif_t *esp_netif = NULL;
void *modem_netif_adapter = NULL;
esp_modem_dce_t *esp_modem_dce = NULL;
static bool timeout_with_4g = false;
/**
 * @brief Macro defined for error checking
 *
 */
#define DCE_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                \
    {                                                                                 \
        if (!(a))                                                                     \
        {                                                                             \
            ESP_LOGE(DCE_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                            \
        }                                                                             \
    } while (0)

/**
 * @brief Handle response from AT+CPOWD=1
 */
static esp_err_t ec600s_handle_power_down(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_POWERDOWN)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    }
    else if (strstr(line,"ERROR"))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    return err;
}

/**
 * @brief Power down
 *
 * @param ec600_dce sim800 object
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
static esp_err_t ec600s_power_down(modem_dce_t *dce)
{
    modem_dte_t *dte = dce->dte;
    int i = 0;
    retry:
    i++;
    vTaskDelay (pdMS_TO_TICKS (500));
    if (i == 5)
    {
        goto err;
    }
    dce->handle_line = ec600s_handle_power_down;
    DCE_CHECK(dte->send_cmd(dte, "AT+CPOWD=1\r", MODEM_COMMAND_TIMEOUT_POWEROFF) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "power down failed", retry);
    ESP_LOGD(DCE_TAG, "power down ok");
    return ESP_OK;
err:
    return ESP_FAIL; 
}

/**
 * @brief Set Working Mode
 *
 * @param dce Modem DCE object
 * @param mode woking mode
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
static esp_err_t ec600s_set_working_mode(modem_dce_t *dce, modem_mode_t mode)
{
    modem_dte_t *dte = dce->dte;
    switch (mode) {
    case MODEM_COMMAND_MODE:
        dce->handle_line = esp_modem_dce_handle_exit_data_mode;
        vTaskDelay(pdMS_TO_TICKS(1000)); // spec: 1s delay for the modem to recognize the escape sequence
        if (dte->send_cmd(dte, "+++", MODEM_COMMAND_TIMEOUT_MODE_CHANGE) != ESP_OK) {
            // "+++" Could fail if we are already in the command mode.
            // in that case we ignore the timeout and re-sync the modem
            ESP_LOGI(DCE_TAG, "Sending \"+++\" command failed");
            dce->handle_line = esp_modem_dce_handle_response_default;
            DCE_CHECK(dte->send_cmd(dte, "AT\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
            DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "sync failed", err);
        } else {
            DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "enter command mode failed", err);
            vTaskDelay(pdMS_TO_TICKS(1000)); // spec: 1s delay after `+++` command
        }
        ESP_LOGD(DCE_TAG, "enter command mode ok");
        dce->mode = MODEM_COMMAND_MODE;
        break;
    case MODEM_PPP_MODE:
        dce->handle_line = esp_modem_dce_handle_atd_ppp;
        DCE_CHECK(dte->send_cmd(dte, "ATD*99*1#\r", MODEM_COMMAND_TIMEOUT_MODE_CHANGE) == ESP_OK, "send command failed", err);
        if (dce->state != MODEM_STATE_SUCCESS) {
            // Initiate PPP mode could fail, if we've already "dialed" the data call before.
            // in that case we retry with "ATO" to just resume the data mode
            ESP_LOGD(DCE_TAG, "enter ppp mode failed, retry with ATO");
            dce->handle_line = esp_modem_dce_handle_atd_ppp;
            DCE_CHECK(dte->send_cmd(dte, "ATO\r", MODEM_COMMAND_TIMEOUT_MODE_CHANGE) == ESP_OK, "send command failed", err);
            DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "enter ppp mode failed", err);
        }
        ESP_LOGD(DCE_TAG, "enter ppp mode ok");
        dce->mode = MODEM_PPP_MODE;
        break;
    default:
        ESP_LOGW(DCE_TAG, "unsupported working mode: %d", mode);
        goto err;
        break;
    }
    return ESP_OK;
err:
    return ESP_FAIL;
}

/**
 * @brief Deinitialize SIM800 object
 *
 * @param dce Modem DCE object
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on fail
 */
static esp_err_t ec600s_deinit(modem_dce_t *dce)
{
    esp_modem_dce_t *esp_modem_dce = __containerof(dce, esp_modem_dce_t, parent);
    if (dce->dte) {
        dce->dte->dce = NULL;
    }
    free(esp_modem_dce);
    return ESP_OK;
}
/**
 * @brief gsm init task
 * 
 * @param arg 
 */
static void gsm_manager_task(void *arg)
{
    ESP_LOGI("EC2x", "\t\r\n--- gsm_manager_task is running ---\r\n");
    // vTaskDelay(2000);
    GSM_Sem = xSemaphoreCreateMutex();
    GSM_Task_tearout = xSemaphoreCreateMutex(); // create tearout gsm
    xSemaphoreTake(GSM_Task_tearout, portMAX_DELAY);
    // xSemaphoreGive(GSM_Task_tearout);
    esp_err_t err;
    uint8_t gsm_init_step = 0;
    bool gsm_init_done = false;
    uint8_t send_at_retry_nb = 0;
    uint8_t retry_time_with4g = 0;
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_PPP();
    timeout_with_4g = false;
    for (uint8_t i = 0; i < 8; i++)
    {
        hw_power_on ();
        vTaskDelay (pdMS_TO_TICKS(1000));
    }
    // xSemaphoreTake(GSM_Task_tearout, portMAX_DELAY);
    for (;;)
    {
        if (!gsm_init_done && esp_modem_dce != NULL)
        {
            switch (gsm_init_step)
            {
                case 0:
                retry_time_with4g++;
                if (retry_time_with4g == 2)
                {
                    
                    timeout_with_4g = true;
                    goto end;
                }
                err = esp_modem_dce_sync(&(esp_modem_dce->parent));
                if (err == ESP_OK)
                {
                    send_at_retry_nb = 0;
                    gsm_init_step++;
                }
                else
                {
                    send_at_retry_nb++;
                    if (send_at_retry_nb > 1)
                    {
                        ESP_LOGE(DCE_TAG, "Modem not response AT command. Reset module...");
                        send_at_retry_nb = 0;
                        reset_GSM();
                    }
                }
                break;
                 case 1:
                err = esp_modem_dce_echo(&(esp_modem_dce->parent), false);
                if (err == ESP_OK)
                {
                    send_at_retry_nb = 0;
                    gsm_init_step++;
                }
                else
                {
                    send_at_retry_nb++;
                    if (send_at_retry_nb > 10)
                    {
                        send_at_retry_nb = 0;
                        gsm_init_step = 0;
                        reset_GSM();
                    }
                }
                break;
                case 2:
                ESP_LOGI(DCE_TAG, "Get module name");
                memset(esp_modem_dce->parent.name, 0, sizeof(esp_modem_dce->parent.name));
                err = esp_modem_dce_get_module_name(&(esp_modem_dce->parent));
                if (err == ESP_OK)
                {
                    ESP_LOGI(DCE_TAG, "GSM software version: %s", esp_modem_dce->parent.name);
                    send_at_retry_nb = 0;
                    gsm_init_step++;
                }
                else
                {
                    send_at_retry_nb++;
                    if (send_at_retry_nb > 10)
                    {
                        ESP_LOGE(DCE_TAG, "Modem not response AT command. Reset module...");
                        send_at_retry_nb = 0;
                        gsm_init_step = 0;
                        reset_GSM();
                    }
                }
                break;
                case 3:
                ESP_LOGI(DCE_TAG, "Get module IMEI");
                memset(esp_modem_dce->parent.imei, 0, sizeof(esp_modem_dce->parent.imei));
                err = esp_modem_dce_get_imei_number (&(esp_modem_dce->parent));
                if (err == ESP_OK)
                {
                    ESP_LOGI(DCE_TAG, "Get GSM IMEI: %s", esp_modem_dce->parent.imei);
                    send_at_retry_nb = 0;
                    gsm_init_step++;
                }
                else
                {
                    send_at_retry_nb++;
                    if (send_at_retry_nb > 10)
                    {
                        ESP_LOGE(DCE_TAG, "Modem not response AT command. Reset module...");
                        send_at_retry_nb = 0;
                        gsm_init_step = 0;
                        reset_GSM();
                    }
                }
                break;
                case 4:
                ESP_LOGI(DCE_TAG, "Get GSM IMSI");
                memset(esp_modem_dce->parent.imsi, 0, sizeof(esp_modem_dce->parent.imsi));
                err = esp_modem_dce_get_imsi_number(&(esp_modem_dce->parent));
                if (err == ESP_OK)
                {
                    ESP_LOGI(DCE_TAG, "Get GSM IMSI: %s", esp_modem_dce->parent.imsi);
                    send_at_retry_nb = 0;
                    gsm_init_step++;
                }
                else
                {
                    send_at_retry_nb++;
                    if (send_at_retry_nb > 10)
                    {
                        ESP_LOGE(DCE_TAG, "Get GSM IMSI: FAILED!");
                        send_at_retry_nb = 0;
                        gsm_init_step++;
                    }
                }
                break;
                case 5:
                ESP_LOGI(DCE_TAG, "check SIM status");
                err = esp_modem_dce_check_sim(&(esp_modem_dce->parent));
                if (err == ESP_OK)
                {
                    ESP_LOGI(DCE_TAG, "SIM ok");
                    gsm_init_step++;
                }
                else
                {
                    send_at_retry_nb++;
                    if (send_at_retry_nb > 10)
                    {
                        ESP_LOGE(DCE_TAG, "SIM FAILED!");
                        send_at_retry_nb = 0;
                        gsm_init_step++;
                    }
                }
                break;
                case 6:
                ESP_LOGI(DCE_TAG, "Get SIM IMEI");
                memset(esp_modem_dce->parent.imei, 0, sizeof(esp_modem_dce->parent.imei));
                err = esp_modem_dce_get_sim_imei (&(esp_modem_dce->parent));
                if (err == ESP_OK)
                {
                    ESP_LOGI(DCE_TAG, "Get SIM IMEI result: %s", esp_modem_dce->parent.imei);
                    send_at_retry_nb = 0;
                    gsm_init_step++;
                }
                else
                {
                    send_at_retry_nb++;
                    if (send_at_retry_nb > 10)
                    {
                        ESP_LOGE(DCE_TAG, "Get SIM IMEI: FAILED!");
                        send_at_retry_nb = 0;
                        gsm_init_step++;
                    }
                }
                break;
                case 7:
                ESP_LOGI(DCE_TAG, "De-active PDP context");
                err = esp_modem_dce_deactive_pdp (&(esp_modem_dce->parent));
                if (err == ESP_OK)
                {
                    send_at_retry_nb = 0;
                    gsm_init_step++;
                }
                else
                {
                    send_at_retry_nb++;
                    if (send_at_retry_nb > 15)
                    {
                        ESP_LOGE(DCE_TAG, "De-active PDP context: FAILED!");
                        send_at_retry_nb = 0;
                        reset_GSM();
                        gsm_init_step++;
                    }
                }
                break;
                case 8:
                ESP_LOGI(DCE_TAG, "Setup APN");
                err = esp_modem_dce_set_apn (&(esp_modem_dce->parent));
                if (err == ESP_OK)
                {
                    send_at_retry_nb = 0;
                    gsm_init_step++;
                }
                else
                {
                    send_at_retry_nb++;
                    if (send_at_retry_nb > 10)
                    {
                        ESP_LOGE(DCE_TAG, "Setup APN: FAILED!");
                        send_at_retry_nb = 0;
                        reset_GSM();
                        gsm_init_step++;
                    }
                }
                break;
                case 9:
                ESP_LOGI(DCE_TAG, "Get network info");
                esp_modem_dce->parent.is_get_net_info = 0;
                err = esp_modem_dce_get_network_info (&(esp_modem_dce->parent));
                if (esp_modem_dce->parent.is_get_net_info == 1)
                {
                    ESP_LOGI(DCE_TAG, "Get network info: OK");
                    send_at_retry_nb = 0;
                    gsm_init_step++;
                }
                else
                {
                    ESP_LOGI(DCE_TAG, "Get network info failed, retry...");
                    send_at_retry_nb++;
                    if (send_at_retry_nb > 30)
                    {
                        ESP_LOGE(DCE_TAG, "Get Network band: FAILED!");
                        send_at_retry_nb = 0;
                        gsm_init_step++;
                    }
                }
                break;
                case 10:
                ESP_LOGI(DCE_TAG, "Register GSM network");
                err = esp_modem_dce_register_network (&(esp_modem_dce->parent));
                if (err == ESP_OK)
                {
                    send_at_retry_nb = 0;
                    gsm_init_step++;
                }
                else
                {
                    send_at_retry_nb++;
                    if (send_at_retry_nb > 10)
                    {
                        ESP_LOGE(DCE_TAG, "Register GSM network: FAILED!");
                        send_at_retry_nb = 0;
                        gsm_init_step++;
                    }
                }
                break;
                case 11:
                send_at_retry_nb = 0;
                /* Print Module ID, Operator, IMEI, IMSI */
                ESP_LOGI(DCE_TAG, "Module name: %s, IMEI: %s", esp_modem_dce->parent.name, esp_modem_dce->parent.imei);
                ESP_LOGI(DCE_TAG, "SIM IMEI: %s, IMSI: %s", esp_modem_dce->parent.imei, esp_modem_dce->parent.imsi);
                ESP_LOGI(DCE_TAG, "Network: %s,%s,%s,%s", esp_modem_dce->parent.oper, esp_modem_dce->parent.act_string,
                esp_modem_dce->parent.network_band, esp_modem_dce->parent.network_channel);

                esp_modem_dce->parent.set_flow_ctrl(&esp_modem_dce->parent, MODEM_FLOW_CONTROL_NONE);
                //	esp_modem_dce->parent.store_profile(&esp_modem_dce->parent);	//sáº½ lÆ°u cáº¥u hÃ¬nh baudrate -> khÃ´ng dÃ¹ng!

                /* Get signal quality */
                uint32_t rssi = 0, ber = 0;
                esp_modem_dce->parent.get_signal_quality(&esp_modem_dce->parent, &rssi, &ber);
                ESP_LOGI(DCE_TAG, "RSSI: %d, Ber: %d", rssi, ber);
                // esp_modem_dce->parent.rssi = rssi;

                /* Get battery voltage */
                uint32_t voltage = 0, bcs = 0, bcl = 0;
                esp_modem_dce->parent.get_battery_status(&esp_modem_dce->parent, &bcs, &bcl, &voltage);
                ESP_LOGI(DCE_TAG, "GSM Vin: %d mV", voltage);
                 if (rssi != 99)
                {
                    /* Only open ppp stack when sim inserted and valid */
                    if (strlen(esp_modem_dce->parent.imsi) >= 15)
                    {
                        esp_modem_dce->parent.imsi[15] = '\0';
                        gsm_init_done = true;

                        xSemaphoreGive(GSM_Sem);
                        
                        /* Setup PPP environment */
                        ESP_LOGI(DCE_TAG, "Start ppp\r\n");
                        
						// assert(ESP_OK == esp_modem_setup_ppp(ec2x_dce->parent.dte));
                        esp_netif = esp_netif_new(&cfg);
                        assert(esp_netif);
                        modem_netif_adapter = esp_modem_netif_setup(esp_modem_dce->parent.dte);
                        ESP_LOGI(DCE_TAG, "netif init");
                        if(esp_modem_netif_set_default_handlers(modem_netif_adapter, esp_netif) == ESP_OK)
                        {
                            ESP_LOGI(DCE_TAG, "register handler");
                        }
                        /* attach the modem to the network interface */
                        if (esp_netif_attach(esp_netif, modem_netif_adapter) == ESP_OK)
                        {
                            ESP_LOGI (DCE_TAG, "NETIF ATTACK OK");
                        }
                        goto end;
                        // assert(ESP_OK == esp_modem_start_ppp(esp_modem_dce->parent.dte));
                    }
                    else
                    {
                        ESP_LOGE(DCE_TAG, "Cannot get sim imei, reset gsm module");
                        send_at_retry_nb = 0;
                        gsm_init_step = 0;
                        gsm_init_done = false;
                        reset_GSM();
                    }
                }
                else
                {
                    gsm_init_step--;
                }
                break;
                default:
            	ESP_LOGE(DCE_TAG, "Unknown step %u\r\n", gsm_init_step);
                break;
            }
        }
            /* Unregister events, destroy the netif adapter and destroy its esp-netif instance */
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    end:
    
    ESP_LOGI("EC2x", "\t\t\r\n--- gsm_manager_task is exit ---");
    xSemaphoreGive(GSM_Task_tearout);
    vTaskDelete(NULL);
}

modem_dce_t *ec600s_init(modem_dte_t *dte)
{
    
    DCE_CHECK(dte, "DCE should bind with a DTE", err);
    /* malloc memory for esp_modem_dce object */
    esp_modem_dce = calloc(1, sizeof(esp_modem_dce_t));
    DCE_CHECK(esp_modem_dce, "calloc esp_modem_dce_t failed", err);
    /* Bind DTE with DCE */
    esp_modem_dce->parent.dte = dte;
    dte->dce = &(esp_modem_dce->parent);
    /* Bind methods */
    esp_modem_dce->parent.handle_line = NULL;
    esp_modem_dce->parent.sync = esp_modem_dce_sync;
    esp_modem_dce->parent.echo_mode = esp_modem_dce_echo;
    esp_modem_dce->parent.store_profile = esp_modem_dce_store_profile;
    esp_modem_dce->parent.set_flow_ctrl = esp_modem_dce_set_flow_ctrl;
    esp_modem_dce->parent.define_pdp_context = esp_modem_dce_define_pdp_context;
    esp_modem_dce->parent.hang_up = esp_modem_dce_hang_up;
    esp_modem_dce->parent.get_signal_quality = esp_modem_dce_get_signal_quality;
    esp_modem_dce->parent.get_battery_status = esp_modem_dce_get_battery_status;
    esp_modem_dce->parent.get_operator_name = esp_modem_dce_get_operator_name;
    esp_modem_dce->parent.set_working_mode = ec600s_set_working_mode;
    esp_modem_dce->parent.power_down = ec600s_power_down;
    esp_modem_dce->parent.deinit = ec600s_deinit;
    // /* Sync between DTE and DCE */
    // DCE_CHECK (esp_modem_dce_sync(&(esp_modem_dce->parent)) == ESP_OK, "sync failed", err_io);
    // /* Close echo */
    // DCE_CHECK (esp_modem_dce_echo(&(esp_modem_dce->parent), false) == ESP_OK, "close echo mode failed", err_io);
    // /* turn on cme error */
    // DCE_CHECK (esp_modem_turn_cmee_on(&(esp_modem_dce->parent), 2) == ESP_OK, "turn cmee got error", err_io);
    // /* Get module info */
    // DCE_CHECK (esp_modem_get_info (&(esp_modem_dce->parent)) == ESP_OK, "get module info fail", err_io);
    // /* send at urc other command q config type */
    // DCE_CHECK (esp_modem_set_urc (&(esp_modem_dce->parent)) == ESP_OK, "set urc error", err_io);
    // /* Config SMS event report */
    // DCE_CHECK (esp_modem_config_sms_event (&(esp_modem_dce->parent)) == ESP_OK, "config sms event", err_io);
    // /* Set SMS text mode */
    // DCE_CHECK (esp_modem_config_text_mode (&(esp_modem_dce->parent)) == ESP_OK, "set text mode fail", err_io); 
    // /* Get Module name */
    // DCE_CHECK (esp_modem_dce_get_module_name(&(esp_modem_dce->parent)) == ESP_OK, "get module name failed", err_io);
    // /* Get IMEI number */
    // DCE_CHECK (esp_modem_dce_get_imei_number(&(esp_modem_dce->parent)) == ESP_OK, "get imei failed", err_io);
    // /* Check sim status*/
    // DCE_CHECK (esp_modem_dce_check_sim(&(esp_modem_dce->parent)) == ESP_OK, "check sim fail", err_io);
    // /* Get IMSI number */
    // DCE_CHECK (esp_modem_dce_get_imsi_number(&(esp_modem_dce->parent)) == ESP_OK, "get imsi failed", err_io);
    // /* Get Sim imei */
    // DCE_CHECK (esp_modem_dce_get_sim_imei (&(esp_modem_dce->parent)) == ESP_OK, "get sim imei fail", err_io);
    // /* Deactive pdp context */
    // DCE_CHECK (esp_modem_dce_deactive_pdp (&(esp_modem_dce->parent)) == ESP_OK, "deactive pdp fail", err_io);
    // /* unlock band */
    // //DCE_CHECK (ec6x_do_unlock_band (&(esp_modem_dce->parent)) == ESP_OK, "unlock band fail", err_io);
    // /* set apn */
    // DCE_CHECK (esp_modem_dce_set_apn (&(esp_modem_dce->parent)) == ESP_OK, "set APN FAIL", err_io);
    // /* Get net work info (oper name) */
    // DCE_CHECK (esp_modem_dce_get_network_info (&(esp_modem_dce->parent)) == ESP_OK, "set APN FAIL", err_io); 
    // /* REGISTER NETWORK */
    // DCE_CHECK (esp_modem_dce_register_network (&(esp_modem_dce->parent)) == ESP_OK, "REGISTER NETWORK FAIL", err_io);
    
    /* Get operator name */
    //DCE_CHECK (esp_modem_dce_get_operator_name(&(esp_modem_dce->parent)) == ESP_OK, "get operator name failed", err_io);
    xTaskCreate(gsm_manager_task, "gsm_manager_task", 4 * 1024, NULL, 5, NULL);
    ESP_LOGI (DCE_TAG,"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    xSemaphoreTake(GSM_Task_tearout, portMAX_DELAY);
    // xSemaphoreTake(GSM_Task_tearout, portMAX_DELAY);
    ESP_LOGI (DCE_TAG,"BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
    if (timeout_with_4g == true)
    {
        goto err;
    }
    
    return &(esp_modem_dce->parent);
// err_io:
//     free(esp_modem_dce);
//     dte->dce = NULL;
err:
    return NULL;
}
