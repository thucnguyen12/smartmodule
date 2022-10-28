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
//#include "app_mqtt.h"
#include "nvs_flash_app.h"
#include "min.h"
#include "min_id.h"
static const char *TAG = "cli";

extern min_context_t m_min_context;
extern void send_min_data(min_msg_t *min_msg);

// static int32_t system_reset (p_shell_context_t context, int32_t argc, char **argv);
// static int32_t get_build_timestamp (p_shell_context_t context, int32_t argc, char **argv);
static int32_t send_mesh_key (p_shell_context_t context, int32_t argc, char **argv);
static int32_t write_data_to_flash_test (p_shell_context_t context, int32_t argc, char **argv);
static int32_t read_data_from_flash_test (p_shell_context_t context, int32_t argc, char **argv);

static const shell_command_context_t cli_command_table[] =
{
         {"meshKey", "\tmeshKey: send a key to test\r\n", send_mesh_key, 4},
         {"test_flash", "\ttest_flash: write fake data to flash\r\n", write_data_to_flash_test, 0},
         {"read_data", "\tread_data: Read data from test flash and display it\r\n", read_data_from_flash_test, 0}
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

static int32_t write_data_to_flash_test (p_shell_context_t context, int32_t argc, char **argv)
{
    static const info_config_t test_config_data = {
        .buzzerEnable = true,
        .charg_interval = 20,
        .httpDnsName = "test",
        .httpDnsPass = "pass",
        .httpDnsPort = 80,
        .httpUsername = "ex_user",
        .inputActiveLevel = 1,
        .mqtt_add = "server_test",
        .mqtt_user = "user",
        .mqtt_pass = "server_pass",
        .networkAddress = "test_addr",
        .pingMainServer = "google.com",
        .pingBackupServer = "google.com",
        .smokeSensorHeartbeatInterval = 100,
        .smokeSensorThresHole = 2,
        .smokeSensorWakeupInterval = 120,
        .syncAlarm = true,
        .tempSensorHeartbeatInterval = 100,
        .tempSensorWakeupInterval = 200,
        .topic_hr = "test_hdr",
        .tempThresHold = 10,
        .uncharg_interval = 2000,
        .userPhoneNumber1 = "0123456789\0",
        .userPhoneNumber2 = "1234567889\0",
        .userPhoneNumber3 = "0192811827\0",
        .wifiDisable = 0,
        .wifiname = "wifiname",
        .wifipass = "pass1234",
        .zoneDelay = 0,
        .zoneMaxMv = 0,
        .zoneMinMv = 0
    };
    esp_err_t err = write_data_to_flash (&test_config_data, sizeof (info_config_t), "test_flash");
    ESP_LOGI (TAG, "WRITE DATA TO FLASH");
    return 0;
}

static int32_t read_data_from_flash_test (p_shell_context_t context, int32_t argc, char **argv)
{
    static info_config_t test_config_data;
    static char str_test [1024];
    uint16_t len = sizeof (info_config_t);
    read_data_from_flash (&test_config_data, &len, "test_flash");
    make_config_info_payload (test_config_data, str_test);
    for (uint8_t i = 0; i < 8; i++)
    {
        ESP_LOGI (TAG, "payload: %s", str_test + 128);
    }
    
    return 0;
}
