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
#include <string.h>
#include "esp_log.h"
#include "esp_modem_dce_service.h"
#include "gsm_ultilities.h"

static const char *DCE_TAG = "dce_service";

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

esp_err_t esp_modem_dce_handle_response_default(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    return err;
}

/**
 * @brief Handle response from AT+CSQ (Get signal quality)
 *
 */
static esp_err_t esp_modem_dce_handle_csq(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    esp_modem_dce_t *esp_dce = __containerof(dce, esp_modem_dce_t, parent);
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    } else if (!strncmp(line, "+CSQ", strlen("+CSQ"))) {
        /* store value of rssi and ber */
        uint32_t **csq = esp_dce->priv_resource;
        /* +CSQ: <rssi>,<ber> */
        sscanf(line, "%*s%d,%d", csq[0], csq[1]);
        err = ESP_OK;
    }
    return err;
}

/**
 * @brief Handle response from AT+CBC (Get battery status)
 *
 */
static esp_err_t esp_modem_dce_handle_cbc(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    esp_modem_dce_t *esp_dce = __containerof(dce, esp_modem_dce_t, parent);
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    } else if (!strncmp(line, "+CBC", strlen("+CBC"))) {
        /* store value of bcs, bcl, voltage */
        uint32_t **cbc = esp_dce->priv_resource;
        /* +CBC: <bcs>,<bcl>,<voltage> */
        sscanf(line, "%*s%d,%d,%d", cbc[0], cbc[1], cbc[2]);
        err = ESP_OK;
    }
    return err;
}

esp_err_t esp_modem_dce_handle_exit_data_mode(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_NO_CARRIER)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    return err;
}

esp_err_t esp_modem_dce_handle_atd_ppp(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_CONNECT)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    return err;
}

/**
 * @brief Handle response from AT+CGMM (Get DCE module name)
 *
 */
static esp_err_t esp_modem_dce_handle_cgmm(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    } else {
        int len = snprintf(dce->name, MODEM_MAX_NAME_LENGTH, "%s", line);
        if (len > 2) {
            /* Strip "\r\n" */
            strip_cr_lf_tail(dce->name, len);
            err = ESP_OK;
        }
    }
    return err;
}

/**
 * @brief Handle response from AT+CGSN (Get DCE module IMEI number)
 *
 */
static esp_err_t esp_modem_dce_handle_cgsn(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    } else {
        int len = snprintf(dce->imei, MODEM_IMEI_LENGTH + 1, "%s", line);
        if (len > 2) {
            /* Strip "\r\n" */
            strip_cr_lf_tail(dce->imei, len);
            err = ESP_OK;
        }
    }
    return err;
}

/**
 * @brief Handle response from AT+CIMI (Get DCE module IMSI number)
 *
 */
static esp_err_t esp_modem_dce_handle_cimi(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    } else {
        int len = snprintf(dce->imsi, MODEM_IMSI_LENGTH + 1, "%s", line);
        if (len > 2) {
            /* Strip "\r\n" */
            strip_cr_lf_tail(dce->imsi, len);
            err = ESP_OK;
        }
    }
    return err;
}

/**
 * @brief Handle response from AT+COPS? (Get Operator's name)
 *
 */
static esp_err_t esp_modem_dce_handle_cops(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    } else if (!strncmp(line, "+COPS", strlen("+COPS"))) {
        /* there might be some random spaces in operator's name, we can not use sscanf to parse the result */
        /* strtok will break the string, we need to create a copy */
        size_t len = strlen(line);
        char *line_copy = malloc(len + 1);
        strcpy(line_copy, line);
        /* +COPS: <mode>[, <format>[, <oper>[, <Act>]]] */
        char *str_ptr = NULL;
        char *p[5];
        uint8_t i = 0;
        /* strtok will broke string by replacing delimiter with '\0' */
        p[i] = strtok_r(line_copy, ",", &str_ptr);
        while (p[i]) {
            p[++i] = strtok_r(NULL, ",", &str_ptr);
        }
        if (i >= 3) {
            int len = snprintf(dce->oper, MODEM_MAX_OPERATOR_LENGTH, "%s", p[2]);
            if (len > 2) {
                /* Strip "\r\n" */
                strip_cr_lf_tail(dce->oper, len);
                err = ESP_OK;
            }
        }
        if (i >= 4) {
            dce->act = (uint8_t)strtol(p[3], NULL, 0);
        }
        free(line_copy);
    }
    return err;
}

static esp_err_t esp_modem_dce_handle_info (modem_dce_t *dce, const char *line)
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
        ESP_LOGI (DCE_TAG, "line info: %s", line);
        return ESP_OK;
    }
    return err;
}
static esp_err_t esp_modem_dce_handle_cpin (modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, "READY"))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    }
    else if (strstr(line, MODEM_RESULT_CODE_ERROR))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    else 
    {
        return ESP_OK;
    }

    return err;   
}

static esp_err_t ec6x_handle_networkband (modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;

    //+QNWINFO: â€œFDD LTEâ€�,46011,â€œLTE BAND 3â€�,1825
    char *net_info_msg = strstr(line, "+QNWINFO:");
    if (net_info_msg != NULL)
    {
        ESP_LOGI(DCE_TAG, "%s", net_info_msg);
        uint8_t comma_idx[6] = {0};
        uint8_t index = 0;

        for (uint8_t i = 0; i < strlen(net_info_msg); i++)
        {
            if (net_info_msg[i] == ',')
                comma_idx[index++] = i;
        }
        if (index >= 3)
        {
            // Copy fields
            memset(dce->act_string, 0, sizeof(dce->act_string));
            char access_tech[64] = {0};
            memcpy(access_tech, &net_info_msg[11], comma_idx[0] - 12);
            snprintf((char *)dce->act_string, 64, "%s", access_tech);

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

            // Network band
            memset(dce->network_band, 0, sizeof(dce->network_band));

            char band_name[64] = {0};
            memcpy(band_name, &net_info_msg[comma_idx[1] + 1], comma_idx[2] - comma_idx[1] - 1);
            snprintf((char *)dce->network_band, 64, "%s", band_name);

            // Network channel
            uint8_t j = 0;
            for (uint8_t i = comma_idx[2] + 1; i < strlen(net_info_msg); i++)
            {
                if (net_info_msg[i] == '\r' || net_info_msg[i] == '\n')
                    break;
                dce->network_channel[j++] = net_info_msg[i];
            }

            ESP_LOGI(DCE_TAG, "Access Technology: %s", dce->act_string);
            ESP_LOGI(DCE_TAG, "Operator code: %s, name: %s", operator_code_name, dce->oper);
            ESP_LOGI(DCE_TAG, "Network band: %s", dce->network_band);
            ESP_LOGI(DCE_TAG, "Network channel: %s", dce->network_channel);

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
    if (strstr (line, "NO SERVICE"))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
        return err;
    }

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
        err = ESP_OK;
    }

    return err;
}


static esp_err_t esp_modem_dce_handle_qccid (modem_dce_t *dce, const char *line)
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
            // ESP_LOGI(TAG, "%s", qccid);
            int len = snprintf(dce->imei, MODEM_IMSI_LENGTH + 1, "%s", &qccid[8]);
            if (len > 2)
            {
                /* Strip "\r\n" */
                strip_cr_lf_tail(dce->imei, len);
                err = ESP_OK;
            }
            // ESP_LOGI(TAG, "SIM IMEI: %s", dce->imei);
        }
    }
    return err;
}

static esp_err_t ec2x_handle_unlockband_step(modem_dce_t *dce, const char *line)
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
    // else 
    // {
    //     return ESP_OK;
    // }
    return err;
}

esp_err_t esp_modem_dce_sync(modem_dce_t *dce)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = esp_modem_dce_handle_response_default;
    DCE_CHECK(dte->send_cmd(dte, "AT\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "sync failed", err);
    ESP_LOGD(DCE_TAG, "sync ok");
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_dce_echo(modem_dce_t *dce, bool on)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = esp_modem_dce_handle_response_default;
    if (on) {
        DCE_CHECK(dte->send_cmd(dte, "ATE1\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
        DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "enable echo failed", err);
        ESP_LOGD(DCE_TAG, "enable echo ok");
    } else {
        DCE_CHECK(dte->send_cmd(dte, "ATE0\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
        DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "disable echo failed", err);
        ESP_LOGD(DCE_TAG, "disable echo ok");
    }
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_dce_store_profile(modem_dce_t *dce)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = esp_modem_dce_handle_response_default;
    DCE_CHECK(dte->send_cmd(dte, "AT&W\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "save settings failed", err);
    ESP_LOGD(DCE_TAG, "save settings ok");
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_dce_set_flow_ctrl(modem_dce_t *dce, modem_flow_ctrl_t flow_ctrl)
{
    modem_dte_t *dte = dce->dte;
    char command[16];
    int len = snprintf(command, sizeof(command), "AT+IFC=%d,%d\r", dte->flow_ctrl, flow_ctrl);
    DCE_CHECK(len < sizeof(command), "command too long: %s", err, command);
    dce->handle_line = esp_modem_dce_handle_response_default;
    DCE_CHECK(dte->send_cmd(dte, command, MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "set flow control failed", err);
    ESP_LOGD(DCE_TAG, "set flow control ok");
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_dce_define_pdp_context(modem_dce_t *dce, uint32_t cid, const char *type, const char *apn)
{
    modem_dte_t *dte = dce->dte;
    char command[64];
    int len = snprintf(command, sizeof(command), "AT+CGDCONT=%d,\"%s\",\"%s\"\r", cid, type, apn);
    DCE_CHECK(len < sizeof(command), "command too long: %s", err, command);
    dce->handle_line = esp_modem_dce_handle_response_default;
    DCE_CHECK(dte->send_cmd(dte, command, MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "define pdp context failed", err);
    ESP_LOGD(DCE_TAG, "define pdp context ok");
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_dce_get_signal_quality(modem_dce_t *dce, uint32_t *rssi, uint32_t *ber)
{
    modem_dte_t *dte = dce->dte;
    esp_modem_dce_t *esp_dce = __containerof(dce, esp_modem_dce_t, parent);
    uint32_t *resource[2] = {rssi, ber};
    esp_dce->priv_resource = resource;
    dce->handle_line = esp_modem_dce_handle_csq;
    DCE_CHECK(dte->send_cmd(dte, "AT+CSQ\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "inquire signal quality failed", err);
    ESP_LOGD(DCE_TAG, "inquire signal quality ok");
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_dce_get_battery_status(modem_dce_t *dce, uint32_t *bcs, uint32_t *bcl, uint32_t *voltage)
{
    modem_dte_t *dte = dce->dte;
    esp_modem_dce_t *esp_dce = __containerof(dce, esp_modem_dce_t, parent);
    uint32_t *resource[3] = {bcs, bcl, voltage};
    esp_dce->priv_resource = resource;
    dce->handle_line = esp_modem_dce_handle_cbc;
    DCE_CHECK(dte->send_cmd(dte, "AT+CBC\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "inquire battery status failed", err);
    ESP_LOGD(DCE_TAG, "inquire battery status ok");
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_dce_get_module_name(modem_dce_t *dce)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = esp_modem_dce_handle_cgmm;
    DCE_CHECK(dte->send_cmd(dte, "AT+CGMM\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "get module name failed", err);
    ESP_LOGD(DCE_TAG, "get module name ok");
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_dce_get_imei_number(modem_dce_t *dce)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = esp_modem_dce_handle_cgsn;
    DCE_CHECK(dte->send_cmd(dte, "AT+CGSN\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "get imei number failed", err);
    ESP_LOGD(DCE_TAG, "get imei number ok");
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_dce_get_imsi_number(modem_dce_t *dce)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = esp_modem_dce_handle_cimi;
    DCE_CHECK(dte->send_cmd(dte, "AT+CIMI\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "get imsi number failed", err);
    ESP_LOGD(DCE_TAG, "get imsi number ok");
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_dce_get_operator_name(modem_dce_t *dce)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = esp_modem_dce_handle_cops;
    DCE_CHECK(dte->send_cmd(dte, "AT+COPS?\r", MODEM_COMMAND_TIMEOUT_OPERATOR) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "get network operator failed", err);
    ESP_LOGD(DCE_TAG, "get network operator ok");
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_dce_hang_up(modem_dce_t *dce)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = esp_modem_dce_handle_response_default;
    DCE_CHECK(dte->send_cmd(dte, "ATH\r", MODEM_COMMAND_TIMEOUT_HANG_UP) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "hang up failed", err);
    ESP_LOGD(DCE_TAG, "hang up ok");
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_turn_cmee_on(modem_dce_t *dce, int mode)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = esp_modem_dce_handle_response_default;
    if (mode == 0)
    {
        DCE_CHECK(dte->send_cmd(dte, "AT+CMEE=0\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
        DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "disable cmee failed", err);
        ESP_LOGD(DCE_TAG, "disable cmee ok");
    }
    else if (mode == 1)
    {
        DCE_CHECK(dte->send_cmd(dte, "AT+CMEE=1\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
        DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "set cmee at level 1 failed", err);
        ESP_LOGD(DCE_TAG, "set cmee at level 1 cmee ok");
    }
    else if (mode == 2)
    {
        DCE_CHECK(dte->send_cmd(dte, "AT+CMEE=2\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
        DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "set cmee at level 2 failed", err);
        ESP_LOGD(DCE_TAG, "set cmee at level 2 cmee ok");
    }
    else
    {
        ESP_LOGE(DCE_TAG, "wrong mode cmee!!");
        goto err;
    }
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_get_info (modem_dce_t *dce)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = esp_modem_dce_handle_info;
    DCE_CHECK(dte->send_cmd(dte, "ATI\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "get module info failed", err);
    ESP_LOGI(DCE_TAG, "get module info ok");
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_set_urc (modem_dce_t *dce)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = esp_modem_dce_handle_response_default;
    DCE_CHECK(dte->send_cmd(dte, "AT+QCFG=\"urc/ri/smsincoming\",\"pulse\",2000\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "set urc fail", err);
    ESP_LOGD(DCE_TAG, "set urc ok");
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_config_sms_event (modem_dce_t *dce)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = esp_modem_dce_handle_response_default;
    DCE_CHECK(dte->send_cmd(dte, "AT+CNMI=2,1,0,0,0\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "config sms event fail", err);       
    return ESP_OK;
err:
    return ESP_FAIL;
}


esp_err_t esp_modem_config_text_mode (modem_dce_t *dce)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = esp_modem_dce_handle_response_default;
    DCE_CHECK(dte->send_cmd(dte, "AT+CMGF=1\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "config text mode event fail", err);       
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_dce_check_sim (modem_dce_t *dce)
{
    modem_dte_t *dte = dce->dte;
    int i = 0;
    retry1:
    i++;
    vTaskDelay(pdMS_TO_TICKS(500));
    if (i == 15)
    {
        goto err;
    }
    dce->handle_line = esp_modem_dce_handle_cpin;
    DCE_CHECK(dte->send_cmd(dte, "AT+CPIN?\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", retry1);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "check sim fail", retry1);
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_dce_get_sim_imei(modem_dce_t *dce)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = esp_modem_dce_handle_qccid;
    DCE_CHECK(dte->send_cmd(dte, "AT+QCCID\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "get sim imei failed", err);
    ESP_LOGD(DCE_TAG, "get sim imei ok");
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t ec6x_do_unlock_band (modem_dce_t *dce)
{
#if 1
    int i = 0;
    retry:
    i++;
    if (i == 7)
    {
        goto err;
    }
    modem_dte_t *dte = dce->dte;
    dce->handle_line = ec2x_handle_unlockband_step;
    DCE_CHECK(dte->send_cmd(dte, "AT+QCFG=\"nwscanseq\",3,1\r", 5000) == ESP_OK, "send command failed", retry);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "Unlock band step0 failed", err);

    dce->handle_line = ec2x_handle_unlockband_step;
    DCE_CHECK(dte->send_cmd(dte, "AT+QCFG=\"nwscanmode\",3,1\r", 5000) == ESP_OK, "send command failed", retry);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "Unlock band step1 failed", err);

    dce->handle_line = ec2x_handle_unlockband_step;
    DCE_CHECK(dte->send_cmd(dte, "AT+QCFG=\"band\",00,45\r", 5000) == ESP_OK, "send command failed", retry);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "Unlock band step2 failed", err);
    ESP_LOGI(DCE_TAG, "Unlock band finished");
    return ESP_OK;
err:
    return ESP_FAIL;
#else
    return ESP_OK;
#endif
}

esp_err_t esp_modem_dce_set_apn (modem_dce_t *dce)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = esp_modem_dce_handle_response_default; 
    DCE_CHECK(dte->send_cmd(dte, "AT+CGDCONT=1,\"IP\",\"m3-world\"\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "set APN failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "send CGREG failed", err);
    ESP_LOGI(DCE_TAG, "Setup APN OK");
    return ESP_OK;
err:
    return ESP_FAIL;    
}

esp_err_t esp_modem_dce_get_network_info (modem_dce_t *dce)
{
    modem_dte_t *dte = dce->dte;
    int i = 0;
    RETRY:
    i++;
    if (i == 10)
    {
        void reset_GSM (void);
        goto err;
    }
    dce->handle_line = ec6x_handle_networkband; 
    DCE_CHECK(dte->send_cmd(dte, "AT+QNWINFO\r", MODEM_COMMAND_TIMEOUT_OPERATOR) == ESP_OK, "send command failed", RETRY);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "get network band failed", err);    
    return ESP_OK;
err:
    return ESP_FAIL;    
}

esp_err_t esp_modem_dce_register_network (modem_dce_t *dce)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = esp_modem_dce_handle_response_default;
    DCE_CHECK(dte->send_cmd(dte, "AT+CGREG=2\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "send CGREG failed", err);
    ESP_LOGI(DCE_TAG, "Register GSM network OK");
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_dce_deactive_pdp (modem_dce_t *dce)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = esp_modem_dce_handle_response_default;   
    DCE_CHECK(dte->send_cmd(dte, "AT+QIDEACT=1\r", MODEM_COMMAND_TIMEOUT_DEFAULT) == ESP_OK, "send command failed", err);
    DCE_CHECK(dce->state == MODEM_STATE_SUCCESS, "send QIDEACT failed", err); 
    ESP_LOGD(DCE_TAG, "de-active pdp ok");
    return ESP_OK;
err:
    return ESP_FAIL;
}

