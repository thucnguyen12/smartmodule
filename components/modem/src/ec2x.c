// Copyright 2015-2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "ec2x.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "netif/ppp/pppapi.h"
#include "netif/ppp/pppos.h"
#include "lwip/dns.h"
// #include "tcpip_adapter.h"
#include "esp_netif.h"
#include "esp_modem.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "gsm_hardware.h"
#include "gsm_ultilities.h"
#include "gsm_hardware.h"
//#include "board.h"
//#include "utilities.h"

#define MODEM_RESULT_CODE_POWERDOWN "ec2x"

#define MODEM_MAX_ACCESS_TECH_STR_LEN   64
#define MODEM_MAX_NETWORK_BAND_STR_LEN  64

static bool timeout_with_4g_ec2x = false;

/**
 * @brief Macro defined for error checking
 *
 */
static const char *TAG = "EC2x";
#define DCE_CHECK(a, str, goto_tag, ...)                                          \
    do                                                                            \
    {                                                                             \
        if (!(a))                                                                 \
        {                                                                         \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                        \
        }                                                                         \
    } while (0)

/**
 * @brief EC2x Modem
 *
 */
typedef struct
{
    void *priv_resource; /*!< Private resource */
    modem_dce_t parent;  /*!< DCE parent class */
} ec2x_modem_dce_t;

ec2x_modem_dce_t *ec2x_dce = NULL;
extern char GSM_IMEI[16];
extern char SIM_IMEI [16];
extern uint8_t csq;
extern SemaphoreHandle_t GSM_Sem;

extern void device_reboot(uint8_t reason);
// extern void gsm_hw_ctrl_power_en(uint8_t ctrl);
// extern void cgsm_hw_ctrl_power_key(bool on)(uint8_t ctrl);
// extern uint8_t CopyParameter(char *BufferSource, char *BufferDes, char FindCharBegin, char FindCharEnd);
// extern uint32_t GetNumberFromString(uint16_t BeginAddress, char *Buffer);

/**
 * @brief Handle response from AT+CSQ
 */
static esp_err_t ec2x_handle_csq(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    ec2x_modem_dce_t *ec2x_dce = __containerof(dce, ec2x_modem_dce_t, parent);
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    }
    else if (strstr(line, MODEM_RESULT_CODE_ERROR))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    else if (!strncmp(line, "+CSQ", strlen("+CSQ")))
    {
        /* store value of rssi and ber */
        uint32_t **csq = ec2x_dce->priv_resource;
        /* +CSQ: <rssi>,<ber> */
        sscanf(line, "%*s%d,%d", csq[0], csq[1]);
        err = ESP_OK;
    }
    return err;
}

/**
 * @brief Handle response from AT+CBC
 */
static esp_err_t ec2x_handle_cbc(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    ec2x_modem_dce_t *ec2x_dce = __containerof(dce, ec2x_modem_dce_t, parent);
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    }
    else if (strstr(line, MODEM_RESULT_CODE_ERROR))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    else if (!strncmp(line, "+CBC", strlen("+CBC")))
    {
        /* store value of bcs, bcl, voltage */
        uint32_t **cbc = ec2x_dce->priv_resource;
        /* +CBC: <bcs>,<bcl>,<voltage> */
        sscanf(line, "%*s%d,%d,%d", cbc[0], cbc[1], cbc[2]);
        err = ESP_OK;
    }
    return err;
}

/**
 * @brief Handle response from +++
 */
static esp_err_t ec2x_handle_exit_data_mode(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    }
    else if (strstr(line, MODEM_RESULT_CODE_NO_CARRIER))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    }
    else if (strstr(line, MODEM_RESULT_CODE_ERROR))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    return err;
}

/**
 * @brief Handle response from ATD*99#
 */
static esp_err_t ec2x_handle_atd_ppp(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_CONNECT))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    }
    else if (strstr(line, MODEM_RESULT_CODE_ERROR))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    return err;
}

// /**
//  * @brief Handle response from AT+CGMM
//  */
// static esp_err_t ec2x_handle_cgmm(modem_dce_t *dce, const char *line)
// {
//     esp_err_t err = ESP_FAIL;
//     if (strstr(line, MODEM_RESULT_CODE_SUCCESS))
//     {
//         err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
//     }
//     else if (strstr(line, MODEM_RESULT_CODE_ERROR))
//     {
//         err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
//     }
//     else
//     {
//         int len = snprintf(dce->name, MODEM_MAX_NAME_LENGTH, "%s", line);
//         if (len > 2)
//         {
//             /* Strip "\r\n" */
//             strip_cr_lf_tail(dce->name, len);
//             err = ESP_OK;
//         }
//     }
//     return err;
// }

/**
 * @brief Handle response from AT+CGMR
 * EC20EQAR01A01E2G
 * OK
 */
static esp_err_t ec2x_handle_cgmr(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    }
    else if (strstr(line, MODEM_RESULT_CODE_ERROR))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    else
    {
        int len = snprintf(dce->name, MODEM_MAX_NAME_LENGTH, "%s", line);
        if (len > 2)
        {
            /* Strip "\r\n" */
            strip_cr_lf_tail(dce->name, len);
            err = ESP_OK;
        }
    }
    return err;
}

/**
 * @brief Handle response from AT+CGSN
 */
static esp_err_t ec2x_handle_cgsn(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    }
    else if (strstr(line, MODEM_RESULT_CODE_ERROR))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    else
    {
        int len = snprintf(dce->imei, MODEM_IMEI_LENGTH + 1, "%s", line);
        if (len > 2)
        {
            /* Strip "\r\n" */
            strip_cr_lf_tail(dce->imei, len);
            err = ESP_OK;
        }
    }
    return err;
}

/**
 * @brief Handle response from AT+CIMI
 * AT+CIMI
 * 460023210226023            //Query IMSI number of SIM which is attached to ME
 * OK
 */
static esp_err_t ec2x_handle_cimi(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    }
    else if (strstr(line, MODEM_RESULT_CODE_ERROR))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    else
    {
        int len = snprintf(dce->imsi, MODEM_IMSI_LENGTH + 1, "%s", line);
        if (len > 2)
        {
            /* Strip "\r\n" */
            strip_cr_lf_tail(dce->imsi, len);
            err = ESP_OK;
        }
    }
    return err;
}

/**
 * @brief Handle response from AT+QCCID
 * +QCCID: 89860025128306012474
 * OK
 */
static esp_err_t ec2x_handle_qccid(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;

    if (strstr(line, MODEM_RESULT_CODE_SUCCESS))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    }
    else if (strstr(line, MODEM_RESULT_CODE_ERROR))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    else
    {
        //        int len = snprintf(dce->imei, MODEM_IMSI_LENGTH + 1, "%s", line);
        //        if (len > 2) {
        //            /* Strip "\r\n" */
        //            strip_cr_lf_tail(dce->imei, len);
        //            err = ESP_OK;
        //        }

        //+QCCID: 89860025128306012474
        char *qccid = strstr(line, "+QCCID:");
        if (qccid != NULL)
        {
            //ESP_LOGI(TAG, "%s", qccid);
            int len = snprintf(dce->imei, MODEM_IMSI_LENGTH + 1, "%s", &qccid[8]);
            if (len > 2)
            {
                /* Strip "\r\n" */
                strip_cr_lf_tail(dce->imei, len);
                err = ESP_OK;
            }
            //ESP_LOGI(TAG, "SIM IMEI: %s", dce->imei);
        }
    }
    return err;
}

/**
 * @brief Handle response from AT+QNINFO
 */
static esp_err_t ec2x_handle_networkband(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;

    //+QNWINFO: â€œFDD LTEâ€�,46011,â€œLTE BAND 3â€�,1825
    char *net_info_msg = strstr(line, "+QNWINFO:");
    if (net_info_msg != NULL)
    {
        ESP_LOGI(TAG, "%s", net_info_msg);

        uint8_t comma_idx[6] = {0};
        uint8_t index = 0;

        for (uint8_t i = 0; i < strlen(net_info_msg); i++)
        {
            if (net_info_msg[i] == ',')
                comma_idx[index++] = i;
        }
        if (index >= 3)
        {
            //Copy fields
            memset(dce->act_string, 0, sizeof(dce->act_string));

            //â€œGSMâ€� â€œGPRSâ€� â€œEDGEâ€�/ â€œWCDMAâ€� â€œHSDPAâ€� â€œHSUPAâ€� â€œHSPA+â€� â€œTDSCDMAâ€�/ â€œTDD LTEâ€� â€œFDD LTEâ€�
            char access_tech[MODEM_MAX_ACCESS_TECH_STR_LEN] = {0};
            memcpy(access_tech, &net_info_msg[10], comma_idx[0] - 10);
            snprintf((char *)dce->act_string, MODEM_MAX_ACCESS_TECH_STR_LEN, "%s", access_tech);

            char operator_code_name[15] = {0};
            memset(dce->oper, 0, sizeof(dce->oper));
            memcpy(operator_code_name, &net_info_msg[comma_idx[0] + 1], comma_idx[1] - comma_idx[0] - 1);
            uint32_t operator_code = gsm_utilities_get_number_from_string(1, operator_code_name);
            switch (operator_code)
            {
            case 45201:
                sprintf(dce->oper, "%s", "MOBIFONE");
                break;
            case 45202:
                sprintf(dce->oper, "%s", "VINAPHONE");
                break;
            case 45204:
                sprintf(dce->oper, "%s", "VIETTEL");
                break;
            case 45205:
                sprintf(dce->oper, "%s", "VNMOBILE");
                break;
            case 45207:
                sprintf(dce->oper, "%s", "BEELINE");
                break;
            case 45208:
                sprintf(dce->oper, "%s", "EVN");
                break;
            default:
                sprintf(dce->oper, "%d", operator_code);
                break;
            }

            //Network band
            memset(dce->network_band, 0, sizeof(dce->network_band));

            char band_name[MODEM_MAX_NETWORK_BAND_STR_LEN] = {0};
            memcpy(band_name, &net_info_msg[comma_idx[1] + 1], comma_idx[2] - comma_idx[1] - 1);
            snprintf((char *)dce->network_band, MODEM_MAX_NETWORK_BAND_STR_LEN, "%s", band_name);

            //Network channel
            uint8_t j = 0;
            for (uint8_t i = comma_idx[2] + 1; i < strlen(net_info_msg); i++)
            {
                if (net_info_msg[i] == '\r' || net_info_msg[i] == '\n')
                    break;
                dce->network_channel[j++] = net_info_msg[i];
            }

            ESP_LOGI(TAG, "Access Technology: %s", dce->act_string);
            ESP_LOGI(TAG, "Operator code: %s, name: %s", operator_code_name, dce->oper);
            ESP_LOGI(TAG, "Network band: %s", dce->network_band);
            ESP_LOGI(TAG, "Network channel: %s", dce->network_channel);

            dce->is_get_net_info = 1;
        }
        else
        {
            if (strstr(net_info_msg, "No Service"))
            {
                sprintf(dce->oper, "%s", "NO SERVICE");
            }
        }
    }

    if (strstr(line, MODEM_RESULT_CODE_SUCCESS))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    }
    else if (strstr(line, MODEM_RESULT_CODE_ERROR))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    else if (strstr (line, "NO SERVICE"))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    else
    {
        err = ESP_OK;
    }

    return err;
}

// /**
//  * @brief Handle response from AT+COPS?
//  */
// static esp_err_t ec2x_handle_cops(modem_dce_t *dce, const char *line)
// {
//     esp_err_t err = ESP_FAIL;
//     if (strstr(line, MODEM_RESULT_CODE_SUCCESS))
//     {
//         err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
//     }
//     else if (strstr(line, MODEM_RESULT_CODE_ERROR))
//     {
//         err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
//     }
//     else if (!strncmp(line, "+COPS", strlen("+COPS")))
//     {
//         /* there might be some random spaces in operator's name, we can not use sscanf to parse the result */
//         /* strtok will break the string, we need to create a copy */
//         size_t len = strlen(line);
//         char *line_copy = malloc(len + 1);
//         strcpy(line_copy, line);
//         /* +COPS: <mode>[, <format>[, <oper>]] */
//         char *str_ptr = NULL;
//         char *p[3];
//         uint8_t i = 0;
//         /* strtok will broke string by replacing delimiter with '\0' */
//         p[i] = strtok_r(line_copy, ",", &str_ptr);
//         while (p[i])
//         {
//             p[++i] = strtok_r(NULL, ",", &str_ptr);
//         }
//         if (i >= 3)
//         {
//             int len = snprintf(dce->oper, MODEM_MAX_OPERATOR_LENGTH, "%s", p[2]);
//             if (len > 2)
//             {
//                 /* Strip "\r\n" */
//                 strip_cr_lf_tail(dce->oper, len);
//                 err = ESP_OK;
//             }
//         }
//         free(line_copy);
//     }
//     return err;
// }

/**
 * @brief Handle response from AT+QPOWD=1
 */
static esp_err_t ec2x_handle_power_down(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS))
    {
        err = ESP_OK;
    }
    else if (strstr(line, MODEM_RESULT_CODE_POWERDOWN))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    }
    return err;
}

// /**
//  * @brief Handle response from any AT command that return result is "OK" 
//  */
// static esp_err_t ec2x_handle_return_OK(modem_dce_t *dce, const char *line)
// {
//     esp_err_t err = ESP_FAIL;
//     if (strstr(line, MODEM_RESULT_CODE_SUCCESS))
//     {
//         err = ESP_OK;
//     }
//     return err;
// }

esp_err_t ec2x_modem_dce_sync(ec2x_modem_dce_t *ec2x_dce)
{
    modem_dte_t *dte = ec2x_dce->parent.dte;
    ec2x_dce->parent.handle_line = esp_modem_dce_handle_response_default;

    if (dte->send_cmd(dte, "ATV1\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_FAIL)
    {
        ESP_LOGE(TAG, "send command failed");
        return ESP_FAIL;
    }
    if (ec2x_dce->parent.state != MODEM_STATE_SUCCESS)
    {
        ESP_LOGE(TAG, "sync failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "sync ok");
    return ESP_OK;
}

esp_err_t ec2x_modem_dce_echo(ec2x_modem_dce_t *ec2x_dce, bool on)
{
    modem_dte_t *dte = ec2x_dce->parent.dte;
    ec2x_dce->parent.handle_line = esp_modem_dce_handle_response_default;

    if (on)
    {
        {
        if (dte->send_cmd(dte, "ATE1\r", MODEM_COMMAND_TIMEOUT_DEFAULT) != ESP_OK)
            ESP_LOGE(TAG, "send command failed");
            return ESP_FAIL;
        }
        if (ec2x_dce->parent.state != MODEM_STATE_SUCCESS)
        {
            ESP_LOGE(TAG, "enable echo failed");
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "enable echo ok");
    }
    else
    {
        if (dte->send_cmd(dte, "ATE0\r", MODEM_COMMAND_TIMEOUT_DEFAULT) != ESP_OK)
        {
            ESP_LOGE(TAG, "send command failed");
            return ESP_FAIL;
        }
        if (ec2x_dce->parent.state != MODEM_STATE_SUCCESS)
        {
            ESP_LOGE(TAG, "disable echo failed");
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "disable echo ok");
    }
    return ESP_OK;
}

/**
 * @brief Get signal quality
 *
 * @param dce Modem DCE object
 * @param rssi received signal strength indication
 * @param ber bit error ratio
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
static esp_err_t ec2x_get_signal_quality(modem_dce_t *dce, uint32_t *rssi, uint32_t *ber)
{
    modem_dte_t *dte = dce->dte;
    ec2x_modem_dce_t *ec2x_dce = __containerof(dce, ec2x_modem_dce_t, parent);
    uint32_t *resource[2] = {rssi, ber};
    ec2x_dce->priv_resource = resource;
    dce->handle_line = ec2x_handle_csq;
    DCE_CHECK(dte->send_cmd(dte, "AT+CSQ\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "inquire signal quality failed", err);
    ESP_LOGD(TAG, "Inquire signal quality OK");
    return ESP_OK;
err:
    return ESP_FAIL;
}

/**
 * @brief Get battery status
 *
 * @param dce Modem DCE object
 * @param bcs Battery charge status
 * @param bcl Battery connection level
 * @param voltage Battery voltage
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
static esp_err_t ec2x_get_battery_status(modem_dce_t *dce, uint32_t *bcs, uint32_t *bcl, uint32_t *voltage)
{
    modem_dte_t *dte = dce->dte;
    ec2x_modem_dce_t *ec2x_dce = __containerof(dce, ec2x_modem_dce_t, parent);
    uint32_t *resource[3] = {bcs, bcl, voltage};
    ec2x_dce->priv_resource = resource;
    dce->handle_line = ec2x_handle_cbc;
    DCE_CHECK(dte->send_cmd(dte, "AT+CBC\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "inquire battery status failed", err);
    ESP_LOGD(TAG, "Inquire gsm Vin status OK");
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
static esp_err_t ec2x_set_working_mode(modem_dce_t *dce, modem_mode_t mode)
{
    modem_dte_t *dte = dce->dte;
    switch (mode)
    {
    case MODEM_COMMAND_MODE:
        dce->handle_line = ec2x_handle_exit_data_mode;
        DCE_CHECK(dte->send_cmd(dte, "+++", MODEM_COMMAND_TIMEOUT_MODE_CHANGE) == ESP_OK, "send command failed", err);
        DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "enter command mode failed", err);
        ESP_LOGI(TAG, "enter command mode OK");
        dce->mode = MODEM_COMMAND_MODE;
        break;
    case MODEM_PPP_MODE:
        dce->handle_line = ec2x_handle_atd_ppp;
        DCE_CHECK(dte->send_cmd(dte, "ATD*99***1#\r", MODEM_COMMAND_TIMEOUT_MODE_CHANGE) == ESP_OK, "send command failed", err);
        DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "enter ppp mode failed", err);
        ESP_LOGI(TAG, "enter ppp mode OK");
        dce->mode = MODEM_PPP_MODE;
        break;
    default:
        ESP_LOGW(TAG, "unsupported working mode: %d", mode);
        goto err;
        break;
    }
    return ESP_OK;
err:
    return ESP_FAIL;
}

/**
 * @brief Power down
 *
 * @param ec2x_dce ec2x object
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
static esp_err_t ec2x_power_down(modem_dce_t *dce)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = ec2x_handle_power_down;
    DCE_CHECK(dte->send_cmd(dte, "AT+QPOWD=1\r", MODEM_COMMAND_TIMEOUT_POWEROFF) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "power down failed", err);
    ESP_LOGI(TAG, "power down OK");
    return ESP_OK;
err:
    return ESP_FAIL;
}

// /**
//  * @brief Get DCE module name
//  *
//  * @param ec2x_dce ec2x object
//  * @return esp_err_t
//  *      - ESP_OK on success
//  *      - ESP_FAIL on error
//  */
// static esp_err_t ec2x_get_module_name(ec2x_modem_dce_t *ec2x_dce)
// {
//     modem_dte_t *dte = ec2x_dce->parent.dte;
//     ec2x_dce->parent.handle_line = ec2x_handle_cgmm;
//     if (dte->send_cmd(dte, "AT+CGMM\r", MODEM_COMMAND_TIMEOUT_DEFAULT) != ESP_OK)
//     {
//         ESP_LOGE(TAG, "send command failed");
//         return ESP_FAIL;
//     }
//     if (ec2x_dce->parent.state != MODEM_STATE_SUCCESS)
//     {
//         ESP_LOGE(TAG, "get module name failed");
//         return ESP_FAIL;
//     }
//     ESP_LOGI(TAG, "get module name ok");
//     return ESP_OK;
// }

/**
 * @brief Request TA Revision Identification of Software Release
 *
 * @param ec2x_dce ec2x object
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
static esp_err_t ec2x_get_module_identify(ec2x_modem_dce_t *ec2x_dce)
{
    modem_dte_t *dte = ec2x_dce->parent.dte;
    ec2x_dce->parent.handle_line = ec2x_handle_cgmr;
    if (dte->send_cmd(dte, "AT+CGMR\r", MODEM_COMMAND_TIMEOUT_DEFAULT) != ESP_OK)
    {
        ESP_LOGE(TAG, "send command failed");
        return ESP_FAIL;
    }
    if (ec2x_dce->parent.state != MODEM_STATE_SUCCESS)
    {
        ESP_LOGE(TAG, "get module identify failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "get module identify OK");
    return ESP_OK;
}

/**
 * @brief Get DCE module IMEI number
 *
 * @param ec2x_dce ec2x object
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
static esp_err_t ec2x_get_imei_number(ec2x_modem_dce_t *ec2x_dce)
{
    modem_dte_t *dte = ec2x_dce->parent.dte;
    ec2x_dce->parent.handle_line = ec2x_handle_cgsn;
    DCE_CHECK(dte->send_cmd(dte, "AT+CGSN\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(ec2x_dce->parent.state == MODEM_STATE_SUCCESS, "get imei number failed", err);
    ESP_LOGI(TAG, "get GSM IMEI OK");
    return ESP_OK;
err:
    return ESP_FAIL;
}

/**
 * @brief Get DCE module IMSI number (SIM IMSI)
 *
 * @param ec2x_dce ec2x object
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
static esp_err_t ec2x_get_imsi_number(ec2x_modem_dce_t *ec2x_dce)
{
    modem_dte_t *dte = ec2x_dce->parent.dte;
    ec2x_dce->parent.handle_line = ec2x_handle_cimi;
    DCE_CHECK(dte->send_cmd(dte, "AT+CIMI\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(ec2x_dce->parent.state == MODEM_STATE_SUCCESS, "get imsi number failed", err);
    ESP_LOGI(TAG, "get SIM IMSI OK");
    return ESP_OK;
err:
    return ESP_FAIL;
}

/**
 * @brief Get DCE module IMEI number (SIM IMEI)
 *
 * @param ec2x_dce ec2x object
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
static esp_err_t ec2x_get_imei(ec2x_modem_dce_t *ec2x_dce)
{
    modem_dte_t *dte = ec2x_dce->parent.dte;
    ec2x_dce->parent.handle_line = ec2x_handle_qccid;
    DCE_CHECK(dte->send_cmd(dte, "AT+QCCID\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(ec2x_dce->parent.state == MODEM_STATE_SUCCESS, "get sim imei failed", err);
    ESP_LOGI(TAG, "get SIM IMEI OK");
    return ESP_OK;
err:
    return ESP_FAIL;
}

// /**
//  * @brief Thiáº¿t láº­p tá»‘c Ä‘á»™ baudrate cho module
//  *
//  * @param ec2x_dce ec2x object
//  * @return esp_err_t
//  *      - ESP_OK on success
//  *      - ESP_FAIL on error
//  */
// static esp_err_t ec2x_set_IPR(ec2x_modem_dce_t *ec2x_dce, uint32_t baudrate)
// {
//     modem_dte_t *dte = ec2x_dce->parent.dte;
//     ec2x_dce->parent.handle_line = esp_modem_dce_handle_response_default;

//     char atcmd[20] = {0};
//     sprintf(atcmd, "AT+IPR=%d\r", baudrate);
//     DCE_CHECK(dte->send_cmd(dte, atcmd, MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
//     DCE_CHECK(ec2x_dce->parent.state == MODEM_STATE_SUCCESS, "set IPR failed", err);
//     ESP_LOGI(TAG, "Thiet lap toc do truyen du lieu OK : %d", baudrate);
//     return ESP_OK;
// err:
//     return ESP_FAIL;
// }

// /**
//  * @brief Thiáº¿t láº­p cháº¿ Ä‘á»™ nháº­n tin nháº¯n SMS
//  *
//  * @param ec2x_dce ec2x object
//  * @return esp_err_t
//  *      - ESP_OK on success
//  *      - ESP_FAIL on error
//  */
// static esp_err_t ec2x_set_CNMI(ec2x_modem_dce_t *ec2x_dce)
// {
//     modem_dte_t *dte = ec2x_dce->parent.dte;
//     ec2x_dce->parent.handle_line = esp_modem_dce_handle_response_default; //ec2x_handle_return_OK;
//     DCE_CHECK(dte->send_cmd(dte, "AT+CNMI=2,1,0,0,0\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
//     DCE_CHECK(ec2x_dce->parent.state == MODEM_STATE_SUCCESS, "set CNMI mode failed", err);
//     ESP_LOGI(TAG, "Thiet lap che do tin nhan OK");
//     return ESP_OK;
// err:
//     return ESP_FAIL;
// }

// /**
//  * @brief Thiáº¿t láº­p SMS á»Ÿ cháº¿ Ä‘á»™ Text
//  *
//  * @param ec2x_dce ec2x object
//  * @return esp_err_t
//  *      - ESP_OK on success
//  *      - ESP_FAIL on error
//  */
// static esp_err_t ec2x_set_CMGF(ec2x_modem_dce_t *ec2x_dce)
// {
//     modem_dte_t *dte = ec2x_dce->parent.dte;
//     ec2x_dce->parent.handle_line = esp_modem_dce_handle_response_default; //ec2x_handle_return_OK;
//     DCE_CHECK(dte->send_cmd(dte, "AT+CMGF=1\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
//     DCE_CHECK(ec2x_dce->parent.state == MODEM_STATE_SUCCESS, "set CMGF failed", err);
//     ESP_LOGI(TAG, "Thiet lap SMS o che do Text OK");
//     return ESP_OK;
// err:
//     return ESP_FAIL;
// }

/**
 * @brief Gá»­i lá»‡nh AT+QIDEACT=1
 *
 * @param ec2x_dce ec2x object
 * @return esp_err_t
 *		- ESP_OK on success
 *		- ESP_FAIL on error
 */
static esp_err_t ec2x_set_QIDEACT(ec2x_modem_dce_t *ec2x_dce)
{
    modem_dte_t *dte = ec2x_dce->parent.dte;
    ec2x_dce->parent.handle_line = esp_modem_dce_handle_response_default; //ec2x_handle_return_OK;
    DCE_CHECK(dte->send_cmd(dte, "AT+QIDEACT=1\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(ec2x_dce->parent.state == MODEM_STATE_SUCCESS, "send QIDEACT failed", err);
    ESP_LOGI(TAG, "De-active PDP OK");
    return ESP_OK;
err:
    return ESP_FAIL;
}

// /**
//  * @brief Gá»­i lá»‡nh AT+QIACT=1
//  *
//  * @param ec2x_dce ec2x object
//  * @return esp_err_t
//  *		- ESP_OK on success
//  *		- ESP_FAIL on error
//  */
// static esp_err_t ec2x_set_QIACT(ec2x_modem_dce_t *ec2x_dce)
// {
//     modem_dte_t *dte = ec2x_dce->parent.dte;
//     ec2x_dce->parent.handle_line = esp_modem_dce_handle_response_default; //ec2x_handle_return_OK;
//     DCE_CHECK(dte->send_cmd(dte, "AT+QIACT=1\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
//     DCE_CHECK(ec2x_dce->parent.state == MODEM_STATE_SUCCESS, "send QIACT failed", err);
//     ESP_LOGI(TAG, "Active PDP OK");
//     return ESP_OK;
// err:
//     return ESP_FAIL;
// }

/**
 * @brief Gá»­i lá»‡nh AT+CGREG=1
 *
 * @param ec2x_dce ec2x object
 * @return esp_err_t
 *		- ESP_OK on success
 *		- ESP_FAIL on error
 */
static esp_err_t ec2x_set_CGREG(ec2x_modem_dce_t *ec2x_dce)
{
    modem_dte_t *dte = ec2x_dce->parent.dte;
    ec2x_dce->parent.handle_line = esp_modem_dce_handle_response_default; //ec2x_handle_return_OK;
    DCE_CHECK(dte->send_cmd(dte, "AT+CGREG=2\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(ec2x_dce->parent.state == MODEM_STATE_SUCCESS, "send CGREG failed", err);
    ESP_LOGI(TAG, "Register GSM network OK");
    return ESP_OK;
err:
    return ESP_FAIL;
}

/**
 * @brief Gá»­i lá»‡nh thiáº¿t láº­p APN
 *
 * @param ec2x_dce ec2x object
 * @return esp_err_t
 *		- ESP_OK on success
 *		- ESP_FAIL on error
 */
static esp_err_t ec2x_set_APN(ec2x_modem_dce_t *ec2x_dce)
{
    modem_dte_t *dte = ec2x_dce->parent.dte;
    ec2x_dce->parent.handle_line = esp_modem_dce_handle_response_default; //ec2x_handle_return_OK;
    DCE_CHECK(dte->send_cmd(dte, "AT+CGDCONT=1,\"IP\",\"v-internet\"\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "set APN failed", err);
    DCE_CHECK(ec2x_dce->parent.state == MODEM_STATE_SUCCESS, "send CGREG failed", err);
    ESP_LOGI(TAG, "Setup APN OK");
    return ESP_OK;
err:
    return ESP_FAIL;
}

// /**
//  * @brief Get Operator's name
//  *
//  * @param ec2x_dce ec2x object
//  * @return esp_err_t
//  *      - ESP_OK on success
//  *      - ESP_FAIL on error
//  */
// static esp_err_t ec2x_get_operator_name(ec2x_modem_dce_t *ec2x_dce)
// {
//     modem_dte_t *dte = ec2x_dce->parent.dte;
//     ec2x_dce->parent.handle_line = ec2x_handle_cops;
//     DCE_CHECK(dte->send_cmd(dte, "AT+COPS?\r", MODEM_COMMAND_TIMEOUT_OPERATOR) == ESP_OK, "send command failed", err);
//     DCE_CHECK(ec2x_dce->parent.state == MODEM_STATE_SUCCESS, "get network operator failed", err);
//     ESP_LOGI(TAG, "get network operator ok");
//     return ESP_OK;
// err:
//     return ESP_FAIL;
// }

/**
 * @brief Get network Band
 *
 * @param ec2x_dce ec2x object
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
static esp_err_t ec2x_get_network_band(ec2x_modem_dce_t *ec2x_dce)
{
    modem_dte_t *dte = ec2x_dce->parent.dte;
    ec2x_dce->parent.handle_line = ec2x_handle_networkband;
    DCE_CHECK(dte->send_cmd(dte, "AT+QNWINFO\r", MODEM_COMMAND_TIMEOUT_OPERATOR) == ESP_OK, "send command failed", err);
    DCE_CHECK(ec2x_dce->parent.state == MODEM_STATE_SUCCESS, "get network band failed", err);
    ESP_LOGI(TAG, "get network band OK");
    return ESP_OK;
err:
    return ESP_FAIL;
}

static void gsm_reset_module(void)
{
    	//Power off module by PowerKey
    	gsm_hw_ctrl_power_key(1);
    	vTaskDelay(650 / portTICK_PERIOD_MS);
    	gsm_hw_ctrl_power_key(0);

    //Turn off Vbat +4V2
//    gsm_hw_ctrl_power_en(0);
    vTaskDelay(3000 / portTICK_PERIOD_MS);

    //Turn ON Vbat +4V2
//    gsm_hw_ctrl_power_en(1);
//    vTaskDelay(1000 / portTICK_PERIOD_MS); //Waiting for Vbat stable 500ms

    /* Turn On module by PowerKey */
    gsm_hw_ctrl_power_key(0);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gsm_hw_ctrl_power_key(1);
}

/******************************************************************************************/
/**
* @brief   : task quáº£n lÃ½ hoáº¡t Ä‘á»™ng cá»§a module gsm
* @param   :  
* @retval  :
* @author  :
* @created :
*/
static void gsm_manager_task(void *arg)
{
    ESP_LOGI("EC2x", "\t\r\n--- gsm_manager_task is running ---\r\n");
    vTaskDelay(2000);
    esp_err_t err;
    uint8_t gsm_init_step = 0;
    bool gsm_init_done = false;
    uint8_t send_at_retry_nb = 0;
    timeout_with_4g_ec2x = false;

    for (;;)
    {
        if (!gsm_init_done && ec2x_dce != NULL)
        {
            switch (gsm_init_step)
            {
            case 0:
                err = esp_modem_dce_sync(&(ec2x_dce->parent));
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
                        ESP_LOGE(TAG, "Modem not response AT command. Reset module...");
                        timeout_with_4g_ec2x = true;
                        send_at_retry_nb = 0;
                        gsm_reset_module();
                        xSemaphoreGive(GSM_Sem);
                    }
                }
                break;
            case 1:
                err = esp_modem_dce_echo(&(ec2x_dce->parent), false);
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
                        gsm_reset_module();
                    }
                }
                break;
                //				case 2:
                //					/* Thay Ä‘á»•i tá»‘c Ä‘á»™ truyá»�n dá»¯ liá»‡u UART baudrate
                //					* Baudrate 115200 cháº¡y á»•n Ä‘á»‹nh khi stream opus
                //					* Baudrate 230400 stream Ä‘Æ°á»£c mp3 128kbps
                //					*/
                //					ESP_LOGI(TAG, "Gui lenh IPR");
                //					err = ec2x_set_IPR(ec2x_dce, 115200);
                //					if(err == ESP_OK) {
                //						/* Thay Ä‘á»•i UART baudrate cá»§a ESP */
                //						if(uart_set_baudrate(UART_NUM_1, 115200) == ESP_OK) {
                //							send_at_retry_nb = 0;
                //							gsm_init_step++;
                //						}
                //					}
                //					break;

            case 2:
                ESP_LOGI(TAG, "Get module name");
                memset(ec2x_dce->parent.name, 0, sizeof(ec2x_dce->parent.name));
                err = ec2x_get_module_identify(ec2x_dce);
                if (err == ESP_OK)
                {
                    ESP_LOGI(TAG, "GSM software version: %s", ec2x_dce->parent.name);
                    send_at_retry_nb = 0;
                    gsm_init_step++;
                }
                else
                {
                    send_at_retry_nb++;
                    if (send_at_retry_nb > 10)
                    {
                        ESP_LOGE(TAG, "Modem not response AT command. Reset module...");
                        send_at_retry_nb = 0;
                        gsm_init_step = 0;
                        gsm_reset_module();
                    }
                }
                break;
            case 3:
                ESP_LOGI(TAG, "Get module IMEI");
                memset(ec2x_dce->parent.imei, 0, sizeof(ec2x_dce->parent.imei));
                err = ec2x_get_imei_number(ec2x_dce);
                if (err == ESP_OK)
                {
                    ESP_LOGI(TAG, "Get GSM IMEI: %s", ec2x_dce->parent.imei);
                    memcpy (GSM_IMEI, (char*) (ec2x_dce->parent.imei), strlen (GSM_IMEI)); //copy IMEI
                    send_at_retry_nb = 0;
                    gsm_init_step++;
                }
                else
                {
                    send_at_retry_nb++;
                    if (send_at_retry_nb > 10)
                    {
                        ESP_LOGE(TAG, "Modem not response AT command. Reset module...");
                        send_at_retry_nb = 0;
                        gsm_init_step = 0;
                        gsm_reset_module();
                    }
                }
                break;
            case 4:
                ESP_LOGI(TAG, "Get GSM IMSI");
                memset(ec2x_dce->parent.imsi, 0, sizeof(ec2x_dce->parent.imsi));
                err = ec2x_get_imsi_number(ec2x_dce);
                if (err == ESP_OK)
                {
                    ESP_LOGI(TAG, "Get GSM IMSI: %s", ec2x_dce->parent.imsi);
                    memcpy (SIM_IMEI, (char*) (ec2x_dce->parent.imsi), strlen (SIM_IMEI)); //copy IMEI
                    //board_set_device_id(atoi(ec2x_dce->parent.imsi));
                    send_at_retry_nb = 0;
                    gsm_init_step++;
                }
                else
                {
                    send_at_retry_nb++;
                    if (send_at_retry_nb > 10)
                    {
                        ESP_LOGE(TAG, "Get GSM IMSI: FAILED!");
                        send_at_retry_nb = 0;
                        gsm_init_step++;
                    }
                }
                break;
            case 5:
                ESP_LOGI(TAG, "Get SIM IMEI");
                memset(ec2x_dce->parent.imei, 0, sizeof(ec2x_dce->parent.imei));
                err = ec2x_get_imei(ec2x_dce);
                if (err == ESP_OK)
                {
                    ESP_LOGI(TAG, "Get SIM IMEI result: %s", ec2x_dce->parent.imei);
                    send_at_retry_nb = 0;
                    gsm_init_step++;
                }
                else
                {
                    send_at_retry_nb++;
                    if (send_at_retry_nb > 10)
                    {
                        ESP_LOGE(TAG, "Get SIM IMEI: FAILED!");
                        send_at_retry_nb = 0;
                        gsm_init_step++;
                    }
                }
                break;

                //				case 6:
                //					ESP_LOGI(TAG, "Gui lenh CNMI: Thiet lap che do SMS");
                //					err = ec2x_set_CNMI(ec2x_dce);
                //					if(err == ESP_OK) {
                //						send_at_retry_nb = 0;
                //						gsm_init_step++;
                //					}
                //					break;
                //				case 7:
                //					ESP_LOGI(TAG, "Gui lenh CMGF: Thiet lap SMS o che do text");
                //					err = ec2x_set_CMGF(ec2x_dce);
                //					if(err == ESP_OK) {
                //						send_at_retry_nb = 0;
                //						gsm_init_step++;
                //					}
                //					break;

            case 6:
                ESP_LOGI(TAG, "De-active PDP context");
                err = ec2x_set_QIDEACT(ec2x_dce);
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
                        ESP_LOGE(TAG, "De-active PDP context: FAILED!");
                        send_at_retry_nb = 0;
                        gsm_init_step++;
                    }
                }
                break;
            case 7:
                ESP_LOGI(TAG, "Setup APN");
                err = ec2x_set_APN(ec2x_dce);
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
                        ESP_LOGE(TAG, "Setup APN: FAILED!");
                        send_at_retry_nb = 0;
                        gsm_init_step++;
                    }
                }
                break;
            case 8:
                /*
					ESP_LOGI(TAG, "Gui lenh QIACT: Active PDP context");
					err = ec2x_set_QIACT(ec2x_dce);
					if(err == ESP_OK) {
						send_at_retry_nb = 0;
						gsm_init_step++;
					} else {
						send_at_retry_nb++;
						if(send_at_retry_nb > 15) {
							ESP_LOGE(TAG, "Active PDP context: FAILED!");
							send_at_retry_nb = 0;
							gsm_init_step++;
						}
					} */

                //Test: KhÃ´ng dÃ¹ng lá»‡nh QIACT ná»¯a Ä‘á»ƒ cháº¡y 2G/3G
                ESP_LOGI(TAG, "Get network info");
                ec2x_dce->parent.is_get_net_info = 0;
                err = ec2x_get_network_band(ec2x_dce);
                if (ec2x_dce->parent.is_get_net_info == 1)
                {
                    ESP_LOGI(TAG, "Get network info: OK");
                    send_at_retry_nb = 0;
                    gsm_init_step++;
                }
                else
                {
                    ESP_LOGI(TAG, "Get network info failed, retry...");
                    send_at_retry_nb++;
                    if (send_at_retry_nb > 30)
                    {
                        ESP_LOGE(TAG, "Get Network band: FAILED!");
                        send_at_retry_nb = 0;
                        gsm_init_step++;
                    }
                }
                break;
            case 9:
                ESP_LOGI(TAG, "Register GSM network");
                err = ec2x_set_CGREG(ec2x_dce);
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
                        ESP_LOGE(TAG, "Register GSM network: FAILED!");
                        send_at_retry_nb = 0;
                        gsm_init_step++;
                    }
                }
                break;
            case 10:
                send_at_retry_nb = 0;

                if (ec2x_dce != NULL)
                {
                    /* Print Module ID, Operator, IMEI, IMSI */
                    ESP_LOGI(TAG, "Module name: %s, IMEI: %s", ec2x_dce->parent.name, ec2x_dce->parent.imei);
                    ESP_LOGI(TAG, "SIM IMEI: %s, IMSI: %s", ec2x_dce->parent.imei, ec2x_dce->parent.imsi);
                    ESP_LOGI(TAG, "Network: %s,%s,%s,%s", ec2x_dce->parent.oper, ec2x_dce->parent.act_string,
                            ec2x_dce->parent.network_band, ec2x_dce->parent.network_channel);

                    ec2x_dce->parent.set_flow_ctrl(&ec2x_dce->parent, MODEM_FLOW_CONTROL_NONE);

                   

                    //ec2x_dce->parent.store_profile(&ec2x_dce->parent);	//sáº½ lÆ°u cáº¥u hÃ¬nh baudrate -> khÃ´ng dÃ¹ng!

                    /* Get signal quality */
                    uint32_t rssi = 0, ber = 0;
                    ec2x_dce->parent.get_signal_quality(&ec2x_dce->parent, &rssi, &ber);
                    ESP_LOGI(TAG, "RSSI: %d, Ber: %d", rssi, ber);
                    //ec2x_dce->parent.rssi = rssi;

                    /* Get battery voltage */
                    uint32_t voltage = 0, bcs = 0, bcl = 0;
                    ec2x_dce->parent.get_battery_status(&ec2x_dce->parent, &bcs, &bcl, &voltage);
                    ESP_LOGI(TAG, "GSM Vin: %d mV", voltage);

                    /* Post event init done to main task to store GSM IMEI */
//                    esp_modem_dte_t *esp_dte = __containerof(ec2x_dce->parent.dte, esp_modem_dte_t, parent);
//                    esp_event_post_to(esp_dte->event_loop_hdl, ESP_MODEM_EVENT, ESP_MODEM_EVENT_INIT_DONE, NULL, 0, 0);
                    // CSQ : NOW
                    csq = rssi;

                    if (rssi != 99)
                    {
						/* Only open ppp stack when sim inserted and valid */
						if (strlen(ec2x_dce->parent.imsi) >= 15)
						{
							ec2x_dce->parent.imsi[15] = '\0';
							gsm_init_done = true;

							/* Setup PPP environment */
							ESP_LOGI(TAG, "Start ppp\r\n");
							assert(ESP_OK == esp_modem_setup_ppp(ec2x_dce->parent.dte));
	                        assert(ESP_OK == esp_modem_start_ppp(ec2x_dce->parent.dte));
						}
						else
						{
							ESP_LOGE(TAG, "Cannot get sim imei, reset gsm module");
							send_at_retry_nb = 0;
							gsm_init_step = 0;
							gsm_init_done = false;
							gsm_reset_module();
						}
                    }
                    else
                    {
                    	gsm_init_step--;
                    }

                }
                else
                {
                    ESP_LOGE(TAG, "ec2x_dce is NULL!");
                    // SoftResetSystem(101);
                    //						esp_restart();
                    device_reboot(101);
                }
                break;

            default:
            	ESP_LOGE(TAG, "Unknown step %u\r\n", gsm_init_step);
                break;
            }
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    ESP_LOGI("EC2x", "\t\t\r\n--- gsm_manager_task is exit ---");
    vTaskDelete(NULL);
}

//static void gsm_manager_task(void *arg)
//{
//    ESP_LOGI("EC2x", "\t\t--- gsm_manager_task is running ---");
//
//	esp_err_t err;
//	/* Sync between DTE and DCE : ATV1 */
//	uint8_t gsm_init_step = 0;
//	bool gsm_init_done = false;
//	uint8_t send_at_retry_nb = 0;
//
//    for (;;) {
//        if(!gsm_init_done && ec2x_dce != NULL) {
//
//			send_at_retry_nb++;
//			if(send_at_retry_nb > 15) {
//				ESP_LOGE(TAG, "Modem not response AT command. Reset module...");
//
//				//Power off module by PowerKey
//				gsm_hw_ctrl_power_key(1);
//				vTaskDelay(1000 / portTICK_PERIOD_MS);
//				gsm_hw_ctrl_power_key(0);
//
//				//Turn off Vbat +4V2
//				gsm_hw_ctrl_power_en(0);
//				vTaskDelay(3000 / portTICK_PERIOD_MS);
//
//				//Turn ON Vbat +4V2
//				gsm_hw_ctrl_power_en(1);
//				vTaskDelay(500 / portTICK_PERIOD_MS);	//Waiting for Vbat stable 500ms
//
//				/* Turn On module by PowerKey */
//				gsm_hw_ctrl_power_key(1);
//				vTaskDelay(1000 / portTICK_PERIOD_MS);
//				gsm_hw_ctrl_power_key(0);
//
//				send_at_retry_nb = 0;
//				gsm_init_step = 0;
//
////				esp_restart();
//			}
//
//			switch(gsm_init_step) {
//				case 0:
//					ESP_LOGI(TAG, "Gui lenh ATV1");
//					err = esp_modem_dce_sync(&(ec2x_dce->parent));
//					if(err == ESP_OK) {
//						send_at_retry_nb = 0;
//						gsm_init_step++;
//					}
//					break;
//				case 1:
//					ESP_LOGI(TAG, "Gui lenh ATE0");
//					err = esp_modem_dce_echo(&(ec2x_dce->parent), false);
//					if(err == ESP_OK) {
//						send_at_retry_nb = 0;
//						gsm_init_step++;
//					}
//					break;
////				case 2:
////					/* Thay Ä‘á»•i tá»‘c Ä‘á»™ truyá»�n dá»¯ liá»‡u UART baudrate
////					* Baudrate 115200 cháº¡y á»•n Ä‘á»‹nh khi stream opus
////					* Baudrate 230400 stream Ä‘Æ°á»£c mp3 128kbps
////					*/
////					ESP_LOGI(TAG, "Gui lenh IPR");
////					err = ec2x_set_IPR(ec2x_dce, 115200);
////					if(err == ESP_OK) {
////						/* Thay Ä‘á»•i UART baudrate cá»§a ESP */
////						if(uart_set_baudrate(UART_NUM_1, 115200) == ESP_OK) {
////							send_at_retry_nb = 0;
////							gsm_init_step++;
////						}
////					}
////					break;
////				case 3:
////					ESP_LOGI(TAG, "Gui lenh CGMM: Get module name");
////					err = ec2x_get_module_name(ec2x_dce);
////					if(err == ESP_OK) {
////						send_at_retry_nb = 0;
////						gsm_init_step++;
////					}
////					break;
//				case 2:
//					ESP_LOGI(TAG, "Gui lenh CGSN: Get module IMEI");
//					memset(ec2x_dce->parent.imei, 0, sizeof(ec2x_dce->parent.imei));
//					err = ec2x_get_imei_number(ec2x_dce);
//					if(err == ESP_OK) {
//						send_at_retry_nb = 0;
//						gsm_init_step++;
//					}
//					break;
//				case 3:
//					ESP_LOGI(TAG, "Gui lenh CIMI: Get SIM IMEI");
//					memset(ec2x_dce->parent.imsi, 0, sizeof(ec2x_dce->parent.imsi));
//					err = ec2x_get_imsi_number(ec2x_dce);
//					if(err == ESP_OK) {
//						send_at_retry_nb = 0;
//						gsm_init_step++;
//					}
//					break;
////				case 6:
////					ESP_LOGI(TAG, "Gui lenh CNMI: Thiet lap che do SMS");
////					err = ec2x_set_CNMI(ec2x_dce);
////					if(err == ESP_OK) {
////						send_at_retry_nb = 0;
////						gsm_init_step++;
////					}
////					break;
////				case 7:
////					ESP_LOGI(TAG, "Gui lenh CMGF: Thiet lap SMS o che do text");
////					err = ec2x_set_CMGF(ec2x_dce);
////					if(err == ESP_OK) {
////						send_at_retry_nb = 0;
////						gsm_init_step++;
////					}
////					break;
//
//				case 4:
//					ESP_LOGI(TAG, "Gui lenh QIDEACT: De-active PDP context");
//					err = ec2x_set_QIDEACT(ec2x_dce);
//					if(err == ESP_OK) {
//						send_at_retry_nb = 0;
//						gsm_init_step++;
//					}
//					break;
//				case 5:
//					ESP_LOGI(TAG, "Gui lenh CGDCONT: Thiet lap APN");
//					err = ec2x_set_APN(ec2x_dce);
//					if(err == ESP_OK) {
//						send_at_retry_nb = 0;
//						gsm_init_step++;
//					}
//					break;
//				case 6:
//					ESP_LOGI(TAG, "Gui lenh QIACT: Active PDP context");
//					err = ec2x_set_QIACT(ec2x_dce);
//					if(err == ESP_OK) {
//						send_at_retry_nb = 0;
//						gsm_init_step++;
//					}
//					break;
//				case 7:
//					ESP_LOGI(TAG, "Gui lenh CGREG: Register GPRS network");
//					err = ec2x_set_CGREG(ec2x_dce);
//					if(err == ESP_OK) {
//						send_at_retry_nb = 0;
//						gsm_init_step++;
//					}
//					break;
//				case 8:
//					gsm_init_done = true;
//					send_at_retry_nb = 0;
//
//					if(ec2x_dce != NULL) {
//						/* Print Module ID, Operator, IMEI, IMSI */
//						ESP_LOGI(TAG, "Module: %s", ec2x_dce->parent.name);
//						ESP_LOGI(TAG, "Operator: %s", ec2x_dce->parent.oper);
//						ESP_LOGI(TAG, "IMEI: %s", ec2x_dce->parent.imei);
//						ESP_LOGI(TAG, "IMSI: %s", ec2x_dce->parent.imsi);
//
//
//						ec2x_dce->parent.set_flow_ctrl(&ec2x_dce->parent, MODEM_FLOW_CONTROL_NONE);
////						ec2x_dce->parent.store_profile(&ec2x_dce->parent);	//sáº½ lÆ°u cáº¥u hÃ¬nh baudrate -> khÃ´ng dÃ¹ng!
//
//						/* Get signal quality */
//						uint32_t rssi = 0, ber = 0;
//						ec2x_dce->parent.get_signal_quality(&ec2x_dce->parent, &rssi, &ber);
//						ESP_LOGI(TAG, "rssi: %d, ber: %d", rssi, ber);
//
//						/* Get battery voltage */
//						uint32_t voltage = 0, bcs = 0, bcl = 0;
//						ec2x_dce->parent.get_battery_status(&ec2x_dce->parent, &bcs, &bcl, &voltage);
//						ESP_LOGI(TAG, "Battery voltage: %d mV", voltage);
//
//						/* post event init done to main task */
//						esp_modem_dte_t *esp_dte = __containerof(ec2x_dce->parent.dte, esp_modem_dte_t, parent);
//						esp_event_post_to(esp_dte->event_loop_hdl, ESP_MODEM_EVENT, MODEM_EVENT_INIT_DONE, NULL, 0, 0);
//
//						/* Setup PPP environment */
//						esp_modem_setup_ppp(ec2x_dce->parent.dte);
//					}else {
//						ESP_LOGE(TAG, "ec2x_dce is NULL!");
//						esp_restart();
//					}
//					break;
//				default:
//					break;
//			}
//    	}
//
//        vTaskDelay(1000 / portTICK_RATE_MS);
//    }
//
//    ESP_LOGI("EC2x", "\t\t--- gsm_manager_task is exit ---");
//    vTaskDelete(NULL);
//}

/**
 * @brief Deinitialize ec2x object
 *
 * @param dce Modem DCE object
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on fail
 */
static esp_err_t ec2x_deinit(modem_dce_t *dce)
{
    ec2x_modem_dce_t *ec2x_dce = __containerof(dce, ec2x_modem_dce_t, parent);
    if (dce->dte)
    {
        dce->dte->dce = NULL;
    }
    free(ec2x_dce);
    return ESP_OK;
}

modem_dce_t *ec2x_init(modem_dte_t *dte)
{
    //	esp_err_t err;
    DCE_CHECK(dte, "DCE should bind with a DTE", err);

    /* malloc memory for ec2x_dce object */
    //    ec2x_modem_dce_t *ec2x_dce = calloc(1, sizeof(ec2x_modem_dce_t));
    ec2x_dce = calloc(1, sizeof(ec2x_modem_dce_t));
    DCE_CHECK(ec2x_dce, "calloc ec2x_dce failed", err);

    /* Bind DTE with DCE */
    ec2x_dce->parent.dte = dte;
    dte->dce = &(ec2x_dce->parent);

    /* Bind methods */
    ec2x_dce->parent.handle_line = NULL;
    ec2x_dce->parent.sync = esp_modem_dce_sync;
    ec2x_dce->parent.echo_mode = esp_modem_dce_echo;
    ec2x_dce->parent.store_profile = esp_modem_dce_store_profile;
    ec2x_dce->parent.set_flow_ctrl = esp_modem_dce_set_flow_ctrl;
    ec2x_dce->parent.define_pdp_context = esp_modem_dce_define_pdp_context;
    ec2x_dce->parent.hang_up = esp_modem_dce_hang_up;
    ec2x_dce->parent.get_signal_quality = ec2x_get_signal_quality;
    ec2x_dce->parent.get_battery_status = ec2x_get_battery_status;
    ec2x_dce->parent.set_working_mode = ec2x_set_working_mode;
    ec2x_dce->parent.power_down = ec2x_power_down;
    ec2x_dce->parent.deinit = ec2x_deinit;


    gsm_hardware_initialize();
    
    gsm_hw_ctrl_power_key(1);
    //Deactive PowerKey
    ESP_LOGI(TAG, "Deactive power key\r\n");
    gsm_hw_ctrl_power_key(0);
//  no need en_pin 
/*
    //Turn off Vbat +4V2
    gsm_hw_ctrl_power_en(0);
    ESP_LOGI(TAG, "Turn off vbat\r\n");
    vTaskDelay(3000 / portTICK_PERIOD_MS);

    //Turn ON Vbat +4V2
    ESP_LOGI(TAG, "Turn on vbat\r\n");
    gsm_hw_ctrl_power_en(1);

*/
    vTaskDelay(500 / portTICK_PERIOD_MS); //Waiting for Vbat stable 500ms

    /* Turn On module by PowerKey */
    ESP_LOGI(TAG, "Turn on power key level 1, pin %d\r\n", CONFIG_CONFIG_GSM_POWER_KEY_PIN);
    gsm_hw_ctrl_power_key(1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
//    ESP_LOGI(TAG, "Turn off power key level 0, pin %d\r\n", CONFIG_CONFIG_GSM_POWER_KEY_PIN);
//    gsm_hw_ctrl_power_key(0);

    ESP_LOGI(TAG, "Start GSM manager task\r\n");
    xTaskCreate(gsm_manager_task, "gsm_manager_task", 4 * 1024, NULL, 5, NULL);
    xSemaphoreTake (GSM_Sem, 20000/ portTICK_PERIOD_MS);
    
//    ESP_LOGI(TAG, "ec2x_init exit!");
    xSemaphoreGive(GSM_Sem);
    if (timeout_with_4g_ec2x)
    {
        goto err;
    }

    return &(ec2x_dce->parent);
//err_io:
//    free(ec2x_dce);
err:
    return NULL;
}
