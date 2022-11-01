/******************************************************************************
 * @file:    app_cli.c
 * @brief:
 * @version: V0.0.0
 * @date:    2019/11/12
 * @author:
 * @email:
 *
 * THE SOURCE CODE AND ITS RELATED DOCUMENTATION IS PROVIDED "AS IS". VINSMART
 * JSC MAKES NO OTHER WARRANTY OF ANY KIND, WHETHER EXPRESS, IMPLIED OR,
 * STATUTORY AND DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF MERCHANTABILITY,
 * SATISFACTORY QUALITY, NON INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * THE SOURCE CODE AND DOCUMENTATION MAY INCLUDE ERRORS. VINSMART JSC
 * RESERVES THE RIGHT TO INCORPORATE MODIFICATIONS TO THE SOURCE CODE IN LATER
 * REVISIONS OF IT, AND TO MAKE IMPROVEMENTS OR CHANGES IN THE DOCUMENTATION OR
 * THE PRODUCTS OR TECHNOLOGIES DESCRIBED THEREIN AT ANY TIME.
 *
 * VINSMART JSC SHALL NOT BE LIABLE FOR ANY DIRECT, INDIRECT OR
 * CONSEQUENTIAL DAMAGE OR LIABILITY ARISING FROM YOUR USE OF THE SOURCE CODE OR
 * ANY DOCUMENTATION, INCLUDING BUT NOT LIMITED TO, LOST REVENUES, DATA OR
 * PROFITS, DAMAGES OF ANY SPECIAL, INCIDENTAL OR CONSEQUENTIAL NATURE, PUNITIVE
 * DAMAGES, LOSS OF PROPERTY OR LOSS OF PROFITS ARISING OUT OF OR IN CONNECTION
 * WITH THIS AGREEMENT, OR BEING UNUSABLE, EVEN IF ADVISED OF THE POSSIBILITY OR
 * PROBABILITY OF SUCH DAMAGES AND WHETHER A CLAIM FOR SUCH DAMAGE IS BASED UPON
 * WARRANTY, CONTRACT, TORT, NEGLIGENCE OR OTHERWISE.
 *
 * (C)Copyright VINSMART JSC 2019 All rights reserved
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "app_cli.h"
#include "app_shell.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "app_spi_flash.h"
// #include "app_drv_spi.h"
#include "gsm_ultilities.h"
#include "app_mqtt.h"
#include "esp_log.h"
//#include "esp_system.h"
//#include "app_flash.h"
//#include "app_ota.h"
#include "app_mqtt.h"
#include "nvs_flash_app.h"
#include "min.h"
#include "min_id.h"
#include "cJSON.h"
#include "http_request.h"
static const char *TAG = "cli";

extern char* key_table;

extern min_context_t m_min_context;
extern void send_min_data(min_msg_t *min_msg);

static cJSON* config_json = NULL;
static cJSON* process_json = NULL;
static info_config_t config_infor_now;
// static int32_t system_reset (p_shell_context_t context, int32_t argc, char **argv);
// static int32_t get_build_timestamp (p_shell_context_t context, int32_t argc, char **argv);
static int32_t send_mesh_key (p_shell_context_t context, int32_t argc, char **argv);
static int32_t write_data_to_flash_test (p_shell_context_t context, int32_t argc, char **argv);
static int32_t read_data_from_flash_test (p_shell_context_t context, int32_t argc, char **argv);
static int32_t set_unicast_addr (p_shell_context_t context, int32_t argc, char **argv);

static const shell_command_context_t cli_command_table[] =
{
         {"meshKey", "\tmeshKey: send a key to test\r\n", send_mesh_key, 4},
         {"test_flash", "\ttest_flash: write fake data to flash\r\n", write_data_to_flash_test, 0},
         {"read_data", "\tread_data: Read data from test flash and display it\r\n", read_data_from_flash_test, 0},
         {"set_unicast", "\tset_unicast: Check and write unicast addr into flash\r\n", set_unicast_addr, 0}
};
static shell_context_struct m_user_context;
static app_cli_cb_t *m_cb;

void app_cli_poll(uint8_t ch)
{
    app_shell_task(ch);
}

void app_cli_start (app_cli_cb_t *callback)
{
    m_cb = callback;
    app_shell_set_context(&m_user_context);
    app_shell_init(&m_user_context,
                   m_cb->puts,
                   m_cb->printf,
                   "",
                   true);

    /* Register CLI commands */
    for (int i = 0; i < sizeof(cli_command_table) / sizeof(shell_command_context_t); i++)
    {
        app_shell_register_cmd(&cli_command_table[i]);
    }

    /* Run CLI task */
    app_shell_task(APP_SHELL_INVALID_CHAR);
}

static int32_t send_mesh_key (p_shell_context_t context, int32_t argc, char **argv)
{
    ble_config_t test_config;
    test_config.config.value = 0xF0;
    memcpy (test_config.appkey, argv[1], 16);
    memcpy (test_config.netkey, argv[2], 16);
    test_config.iv_index = gsm_utilities_get_number_from_string(0, *argv);
    test_config.sequence_number = gsm_utilities_get_number_from_string(0, *argv);
    // TODO:  send min packet
    min_msg_t test_config_msg;
    test_config_msg.id = MIN_ID_SEND_KEY_CONFIG;
    test_config_msg.payload = &test_config;
    test_config_msg.len = sizeof (ble_config_t);
    send_min_data (&test_config_msg);
    return 0;
}

static void process_config_data (type_of_mqtt_data_t data_type, mqtt_config_list mqtt_config_list , void* string_value, int int_value, bool bool_value)
{
    static char string_key[32];
    make_key_to_store_type_in_nvs (string_key, mqtt_config_list);
    switch (data_type)
    {
    case STRING_TYPE:
        if (string_value != NULL)
        {
            if (mqtt_config_list == TOPIC_HDR)
            {
                memcpy (config_infor_now.topic_hr, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == MQTT_ADDR)
            {
                memcpy (config_infor_now.mqtt_add, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == MQTT_USER)
            {
                memcpy (config_infor_now.mqtt_user, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == MQTT_PW)
            {
                memcpy (config_infor_now.mqtt_pass, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == PHONE_NUM_1)
            {
                memcpy (config_infor_now.userPhoneNumber1, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == PHONE_NUM_2)
            {
                memcpy (config_infor_now.userPhoneNumber2, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == PHONE_NUM_3)
            {
                memcpy (config_infor_now.userPhoneNumber3, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == DNS_NAME)
            {
                memcpy (config_infor_now.httpDnsName, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == DNS_USER)
            {
                memcpy (config_infor_now.httpUsername, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == DNS_PASS)
            {
                memcpy (config_infor_now.httpDnsPass, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == WIFI_NAME)
            {
                memcpy (config_infor_now.wifiname, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == WIFI_PASS)
            {
                memcpy (config_infor_now.wifipass, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == PING_MAIN_SERVER)
            {
                memcpy (config_infor_now.pingMainServer, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == PING_BACKUP_SERVER)
            {
                memcpy (config_infor_now.pingBackupServer, string_value, strlen ((char*)string_value));
            }
            // ghi data vao flash
            
            internal_flash_nvs_write_string (string_key, string_value);
        }
        break;
    case INT_TYPE:
        if (mqtt_config_list == CHARG_INTERVAL)
        {
            config_infor_now.charg_interval = (uint16_t) int_value;
        }
        
        if (mqtt_config_list == UNCHARG_INTERVAL)
        {
            config_infor_now.uncharg_interval = (uint16_t) int_value;
        }
        
        if (mqtt_config_list == SMOKE_WAKE)
        {
            config_infor_now.smokeSensorWakeupInterval = (uint16_t) int_value;
        }
        
        if (mqtt_config_list == SMOKE_HEARTBEAT)
        {
            config_infor_now.smokeSensorHeartbeatInterval = (uint16_t) int_value;
        }
        
        if (mqtt_config_list == SMOKE_THRESH)
        {
            config_infor_now.smokeSensorThresHole = (uint16_t) int_value;
        }
        
        if (mqtt_config_list == TEMPER_WAKE)
        {
            config_infor_now.tempSensorWakeupInterval = (uint16_t) int_value;
        }
        
        if (mqtt_config_list == TEMPER_HEARTBEAT)
        {
            config_infor_now.tempSensorHeartbeatInterval = (uint16_t) int_value;
        }

        if (mqtt_config_list == TEMPER_THRESH)
        {
            config_infor_now.tempThresHold = (uint16_t) int_value;
        }
       
        internal_flash_nvs_write_u16 (string_key, int_value);
        break;
    case BOOL_TYPE:
        if (mqtt_config_list == BUZZ_EN)
        {
            config_infor_now.buzzerEnable = bool_value;
        }
        else if (mqtt_config_list == SYNC_ALARM)
        {
            config_infor_now.syncAlarm = bool_value;
        }
        
        internal_flash_nvs_write_u8 (string_key, (uint8_t)bool_value);
        break;
    default:
        break;
    }
}

static int32_t write_data_to_flash_test (p_shell_context_t context, int32_t argc, char **argv)
{
    // static const info_config_t test_config_data = {
    //     .buzzerEnable = true,
    //     .charg_interval = 20,
    //     .httpDnsName = "test",
    //     .httpDnsPass = "pass",
    //     .httpDnsPort = 80,
    //     .httpUsername = "ex_user",
    //     .inputActiveLevel = 1,
    //     .mqtt_add = "server_test",
    //     .mqtt_user = "user",
    //     .mqtt_pass = "server_pass",
    //     .networkAddress = "test_addr",
    //     .pingMainServer = "google.com",
    //     .pingBackupServer = "google.com",
    //     .smokeSensorHeartbeatInterval = 100,
    //     .smokeSensorThresHole = 2,
    //     .smokeSensorWakeupInterval = 120,
    //     .syncAlarm = true,
    //     .tempSensorHeartbeatInterval = 100,
    //     .tempSensorWakeupInterval = 200,
    //     .topic_hr = "test_hdr",
    //     .tempThresHold = 10,
    //     .uncharg_interval = 2000,
    //     .userPhoneNumber1 = "0123456789\0",
    //     .userPhoneNumber2 = "1234567889\0",
    //     .userPhoneNumber3 = "0192811827\0",
    //     .wifiDisable = 0,
    //     .wifiname = "wifiname",
    //     .wifipass = "pass1234",
    //     .zoneDelay = 0,
    //     .zoneMaxMv = 0,
    //     .zoneMinMv = 0
    // };
    // esp_err_t err = write_data_to_flash (&test_config_data, sizeof (info_config_t), "test_flash");
    
    static char* configJsonstr = "{\"topicHeader\": \"string_ex\"\r\n,\"mqttAddress\": \"string_ex\"\r\n,\"mqttUserName\": \"string_ex\"\r\n,\"mqttPassword\": \"string_ex\"\r\n,\"chargingInterval\": 100\r\n,\"unchargeInterval\": 100\r\n,\"userPhoneNumber1\": \"1234567890\"\r\n,\"userPhoneNumber2\": \"1234567890\"\r\n,\"userPhoneNumber3\": \"1213154245\"\r\n,\"buzzerEnable\": true\r\n,\"syncAlarm\": true\r\n,\"networkAddress\": \"string_ex\"\r\n,\"smokeSensorWakeupInterval\": 100\r\n,\"smokeSensorHeartbeatInterval\": 100\r\n,\"smokeSensorThresHole\": 100\r\n,\"tempSensorWakeupInterval\": 100\r\n,\"tempSensorHeartbeatInterval\": 100\r\n,\"tempThresHold\": 100\r\n,\"httpDnsName\":\"string_ex\"\r\n,\"httpUsername\":\"string_ex\"\r\n,\"httpDnsPass\":\"string_ex\"\r\n,\"httpDnsPort\":10\r\n,\"wifiname\":\"string_ex\"\r\n,\"wifipass\":\"string_ex\"\r\n,\"wifiDisable\":100\r\n,\"reset\":0\r\n,\"pingMainServer\":\"string_ex\"\r\n,\"pingBackupServer\":\"string_ex\"\r\n,\"inputActiveLevel\":1\r\n,\"zoneMinMv\":0\r\n,\"zoneMaxMv\":0\r\n,\"zoneDelay\":0\r\n}";
    

    config_json = NULL;
    // char* config = strstr (event->data, "{");
    config_json = parse_json (configJsonstr);
    if (config_json == NULL)
    {
        // if not found string
        ESP_LOGI (TAG, "NULL detected");
        
    }
    for (mqtt_config_list i = 0; i < MAX_CONFIG_HANDLE; i++)
    {
        
        process_json = NULL;
        static char string_key[64];
        memset (string_key,'\0', 64);
        make_key_from_mqtt_list_type(string_key, i);
        ESP_LOGI (TAG, "STRING KEY : %s", string_key);
        process_json = cJSON_GetObjectItemCaseSensitive (config_json, string_key);
            
        if (process_json == NULL)
        {
            // if not found string
            ESP_LOGE (TAG,"NOT FOUND STR %s", string_key);
            continue;
        }
        type_of_mqtt_data_t type_of_process_data = get_type_of_data (i);
        if (type_of_process_data == STRING_TYPE)
        {
            if (cJSON_IsString(process_json))
            {
                process_config_data(type_of_process_data, i, process_json->valuestring, 0 , 0);
            }
        }
        else if (type_of_process_data == INT_TYPE)
        {
            if (cJSON_IsNumber(process_json))
            {
                process_config_data(type_of_process_data, i, NULL, process_json->valueint , 0);
            }
        }
        else if (type_of_process_data == BOOL_TYPE)
        {
            if (cJSON_IsBool(process_json) == cJSON_True)
            {
                process_config_data(type_of_process_data, i, NULL, 0, true);
            }
            else if (cJSON_IsBool(process_json) == cJSON_False)
            {
                process_config_data(type_of_process_data, i, NULL, 0, false);
            }
        }
    }
    ESP_LOGI (TAG, "WRITE DATA TO FLASH");
    return 0;

}

static int32_t read_data_from_flash_test (p_shell_context_t context, int32_t argc, char **argv)
{
    static info_config_t test_config_data;
    static char str_test [1024];
    static char str_key [32];
    static type_of_mqtt_data_t type_of_process_data;
    static esp_err_t err;
    // read_data_from_flash (&test_config_data, &len, "test_flash");
    for (mqtt_config_list i = 0; i < MAX_CONFIG_HANDLE; i++)
    {
        type_of_process_data = get_type_of_data (i);
        err = read_config_data_from_flash (&test_config_data, type_of_process_data, i);
        if (err != ESP_OK)
        {
            make_key_to_store_type_in_nvs (str_key, i);
            ESP_LOGI (TAG, "GOT ERR %d WHILE READING %s KEY", err, str_key);
        }
    }
    memset (str_test, '\0', 1024);
    make_config_info_payload (test_config_data, str_test);
    // for (uint8_t i = 0; i < 2; i++)
    // {
        ESP_LOGI (TAG, "payload: %s", str_test);
    // }
    
    return 0;
}


static int32_t set_unicast_addr (p_shell_context_t context, int32_t argc, char **argv)
{
    static char mac_key [64];
    beacon_pair_info_t pair_info = {
        .device_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
        .device_type = 1,
        .pair_success = false
    };
    uint16_t unicast_addr;
    node_sensor_data_t node_data;
    memcpy (node_data.device_mac, pair_info.device_mac, 6);
    node_data.device_type = pair_info.device_type;
    build_string_from_MAC (node_data.device_mac, mac_key);
    ESP_LOGI (TAG, "mac string: %s", mac_key);
    // write
    size_t len = sizeof (node_sensor_data_t);
    unicast_addr = find_and_write_into_mac_key_space(&node_data, &len, mac_key);

    node_sensor_data_t sensor_data_response;
    memcpy (sensor_data_response.device_mac, pair_info.device_mac, 6);
    sensor_data_response.device_type = pair_info.device_type;
    sensor_data_response.unicast_add = unicast_addr;
    // printf du lieu doc ra
    ESP_LOGI (TAG, "MAC: %02x:%02x:%02x:%02x:%02x:%02x", sensor_data_response.device_mac[0],
                                                        sensor_data_response.device_mac[1],
                                                        sensor_data_response.device_mac[2],
                                                        sensor_data_response.device_mac[3],
                                                        sensor_data_response.device_mac[4],
                                                        sensor_data_response.device_mac[5]);
    ESP_LOGI (TAG, "DEVICE TYPE: %d", sensor_data_response.device_type);
    ESP_LOGI (TAG, "UNICAST ADDR: %04x", sensor_data_response.unicast_add);
    return 0;
}
