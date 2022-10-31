/* PPPoS Client Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_netif.h"
#include "esp_netif_ppp.h"
#include "mqtt_client.h"
#include "esp_modem.h"
#include "esp_modem_netif.h"
#include "esp_log.h"
#include "sim800.h"
#include "bg96.h"
#include "sim7600.h"
#include "hw_power.h"
#include "driver/gpio.h"
#include "ec600s.h"
#include "ec2x.h"
#include "sdkconfig.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "lwip/err.h"
#include "lwip/sys.h"
//wifi
#include "app_wifi.h"
#include "ping/ping_sock.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
///http ota
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "app_ota.h"

//tiny usb (for esp32-s2 only)
#ifdef ESP32_S2
#include "tinyusb.h" 
#include "tusb_cdc_acm.h"
#endif

//RTC
#include "rtc.h"
#include "esp_sntp.h"
#include "hal/cpu_hal.h"
#include "hal/emac_hal.h"
#include "esp_eth.h"
#include "esp_eth_mac.h"
//watchdog task
#include "esp_task_wdt.h"

#if CONFIG_ETH_USE_SPI_ETHERNET
#include "driver/spi_master.h"
#endif // CONFIG_ETH_USE_SPI_ETHERNET
// uart needed
#include "lwrb.h"
#include "gpio_defination.h"
#include "min.h"
#include "min_id.h"
//http request
#include "http_request.h"
//sync timer
#include "app_time_sync.h"
#include "gsm_ultilities.h"
//
#include "app_mqtt.h"
#include "cJSON.h"
//#include "spi_eeprom.h"
#include "app_uart.h"
//app cli
#include "app_cli.h"

#include "esp_netif_types.h"
#include "esp_netif_defaults.h"

#include "nvs_flash_app.h"

#define BROKER_URL "mqtt://mqtt.eclipseprojects.io:1883"
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#define IS_WIFI_DISABLE 1


#define FIRMWARE_VERSION "0x01"
#define HARDWARE_VERSION "0x01"

#define DEFAULT_PROTOCOL WIFI_PROTOCOL

#define EXAMPLE_PING_IP            "www.google.com"
#define BACKUP_PING_IP            "www.google.com"
#define EXAMPLE_PING_COUNT         5
#define EXAMPLE_PING_INTERVAL      1
#define BUF_SIZE (1024)

#define TWDT_TIMEOUT_S          30
#define TASK_RESET_PERIOD_S     2

#define CONFIG_EXAMPLE_SKIP_VERSION_CHECK 1
#define UART_RINGBUFF_SIZE 1024

#define EEPROM_HOST     HSPI_HOST
#define PIN_NUM_MISO    18
#define PIN_NUM_MOSI    23
#define PIN_NUM_CLK     19
#define PIN_NUM_CS 0     13

extern char* key_table;




//NVS FLASH
#define NVS_CONFIG_KEY "config_flash_region"
#define NVS_DATA_PING "ping_data: %d"
#define NVS_DATA_SENSOR "data_sensor: %d"
static uint8_t ping_data_count_in_flash = 0;
char ping_data_key [32];
static uint8_t data_sensor_count_in_flash = 0;
char data_sensor_key [32];


static const char *TAG = "pppos_example";
//group event and bits
static EventGroupHandle_t event_group = NULL;
static const int CONNECT_BIT = BIT0;
static const int STOP_BIT = BIT1;
static const int GOT_DATA_BIT = BIT2;
static const int MQTT_DIS_CONNECT_BIT = BIT3;
static const int UPDATE_BIT = BIT4; 
static const int MQTT_CONNECT_BIT = BIT5;

//TaskHandle_t m_uart_task = NULL;
//semaphore  is needed
SemaphoreHandle_t GSM_Sem;
SemaphoreHandle_t GSM_Task_tearout;
//some extern variable
// 2 line is from ec600
//extern esp_netif_t *esp_netif;
//extern void *modem_netif_adapter;
// list netif
extern esp_netif_t *wifi_netif;
extern esp_netif_t *gsm_esp_netif;
esp_netif_t *eth_netif;

extern EventGroupHandle_t s_wifi_event_group;
modem_dte_t *dte = NULL;
extern char ota_url [128];
// static bool network_connected = false;
// extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
// extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");
// imei
char GSM_IMEI [16] = "not init yet";
char SIM_IMEI [16];
uint8_t csq;

extern QueueHandle_t uart1_queue;
extern QueueHandle_t uart0_queue;
QueueHandle_t mqtt_info_queue;

lwrb_t data_uart_module_rb;
uint8_t min_rx_buffer [UART_RINGBUFF_SIZE];
char uart_buffer [UART_RINGBUFF_SIZE];
min_context_t m_min_context;
static min_frame_cfg_t m_min_setting = MIN_DEFAULT_CONFIG();
// static xSemaphoreHandle s_semph_get_ip_addrs;

static const min_msg_t ping_min_msg = {
    .id = MIN_ID_PING_ESP_ALIVE,
    .len = 0,
    .payload = NULL
};


typedef enum
{
    GSM_4G_PROTOCOL,
    WIFI_PROTOCOL,
    ETHERNET_PROTOCOL,
    PROTOCOL_NONE
} protocol_type;
protocol_type protocol_using = DEFAULT_PROTOCOL;
static char* wifi_name = CONFIG_ESP_WIFI_SSID;
static char* wifi_pass = CONFIG_ESP_WIFI_PASSWORD;

esp_mqtt_client_config_t mqtt_config;
static char mqtt_host [64];
uint32_t mqtt_port;
static char mqtt_username [64];
static char mqtt_password [64];

char heart_beat_topic_header [64];
char fire_alarm_topic_header [64];
char sensor_topic_header [64];
char info_topic_header [64];
char config_topic_header [64];
char ota_header [64];


//min relate ble handle variable
uint8_t payload_buffer [256];
app_beacon_ping_msg_t* beacon_ping_data_handle;
app_beacon_data_t* beacon_data_handle;
fire_status_t alarm_status;
sensor_info_t sensor_info;

char mqtt_payload [512];
char* ota_server_str_buff;

static info_config_t config_infor_now;
// info_config_from_server_t config_infor_from_server;

//extern char recv_buf[64];
//General variable
esp_mqtt_client_handle_t mqtt_client;
esp_eth_handle_t eth_handle;
modem_dce_t *dce = NULL;
bool mqtt_server_ready = false;
bool wifi_started = false;
bool eth_started = false;
bool gsm_started = false;

void send_min_data(min_msg_t *min_msg);
// esp_err_t read_data_from_flash (void *data_read, uint16_t byte_read, const char* key);
// int32_t write_data_to_flash(void *data_write, size_t byte_write, const char* key);
// void init_nvs_flash(void );
void send_current_config (info_config_t config_infor_now);

//********************* APP CLI VARIABLE ****************************//
uint32_t cdc_tx(const void *buffer, uint32_t size);
int32_t USB_puts(char *msg);

void cli_cdc_tx(uint8_t *buffer, uint32_t size);
int cli_cdc_puts(const char *msg);

static app_cli_cb_t m_tcp_cli =
{
	.puts = cli_cdc_tx,
	.printf = cli_cdc_puts,
	.terminate = NULL
};

static bool m_cli_started = false;
// ******************************************************************//

void device_reboot(uint8_t reason)
{
    (void)reason;
    ESP_LOGE(TAG, "Device reboot reason %u", reason);
    vTaskDelay(1000);
    esp_restart();
}

#if CONFIG_EXAMPLE_SEND_MSG
/**
 * @brief This example will also show how to send short message using the infrastructure provided by esp modem library.
 * @note Not all modem support SMG.
 *
 */
static esp_err_t example_default_handle(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    return err;
}

static esp_err_t example_handle_cmgs(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    } else if (!strncmp(line, "+CMGS", strlen("+CMGS"))) {
        err = ESP_OK;
    }
    return err;
}

#define MODEM_SMS_MAX_LENGTH (128)
#define MODEM_COMMAND_TIMEOUT_SMS_MS (120000)
#define MODEM_PROMPT_TIMEOUT_MS (100)

static esp_err_t example_send_message_text(modem_dce_t *dce, const char *phone_num, const char *text)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = example_default_handle;
    /* Set text mode */
    if (dte->send_cmd(dte, "AT+CMGF=1\r", MODEM_COMMAND_TIMEOUT_DEFAULT) != ESP_OK) {
        ESP_LOGE(TAG, "send command failed");
        goto err;
    }
    if (dce->state != MODEM_STATE_SUCCESS) {
        ESP_LOGE(TAG, "set message format failed");
        goto err;
    }
    ESP_LOGD(TAG, "set message format ok");
    /* Specify character set */
    dce->handle_line = example_default_handle;
    if (dte->send_cmd(dte, "AT+CSCS=\"GSM\"\r", MODEM_COMMAND_TIMEOUT_DEFAULT) != ESP_OK) {
        ESP_LOGE(TAG, "send command failed");
        goto err;
    }
    if (dce->state != MODEM_STATE_SUCCESS) {
        ESP_LOGE(TAG, "set character set failed");
        goto err;
    }
    ESP_LOGD(TAG, "set character set ok");
    /* send message */
    char command[MODEM_SMS_MAX_LENGTH] = {0};
    int length = snprintf(command, MODEM_SMS_MAX_LENGTH, "AT+CMGS=\"%s\"\r", phone_num);
    /* set phone number and wait for "> " */
    dte->send_wait(dte, command, length, "\r\n> ", MODEM_PROMPT_TIMEOUT_MS);
    /* end with CTRL+Z */
    snprintf(command, MODEM_SMS_MAX_LENGTH, "%s\x1A", text);
    dce->handle_line = example_handle_cmgs;
    if (dte->send_cmd(dte, command, MODEM_COMMAND_TIMEOUT_SMS_MS) != ESP_OK) {
        ESP_LOGE(TAG, "send command failed");
        goto err;
    }
    if (dce->state != MODEM_STATE_SUCCESS) {
        ESP_LOGE(TAG, "send message failed");
        goto err;
    }
    ESP_LOGD(TAG, "send message ok");
    return ESP_OK;
err:
    return ESP_FAIL;
}
#endif

static void cmd_ping_on_ping_success(esp_ping_handle_t hdl, void *args)
{
    uint8_t ttl;
    uint16_t seqno;
    uint32_t elapsed_time, recv_len;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
    printf("%d bytes from %s icmp_seq=%d ttl=%d time=%d ms\n",
    recv_len, inet_ntoa(target_addr.u_addr.ip4), seqno, ttl, elapsed_time);
}

static void cmd_ping_on_ping_timeout(esp_ping_handle_t hdl, void *args)
{
    uint16_t seqno;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    printf("From %s icmp_seq=%d timeout\n", inet_ntoa(target_addr.u_addr.ip4), seqno);
    xEventGroupSetBits(s_wifi_event_group, WIFI_PING_TIMEOUT);
}


static void cmd_ping_on_ping_end(esp_ping_handle_t hdl, void *args)
{
    ip_addr_t target_addr;
    uint32_t transmitted;
    uint32_t received;
    uint32_t total_time_ms;
    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
    uint32_t loss = (uint32_t)((1 - ((float)received) / transmitted) * 100);
    if (IP_IS_V4(&target_addr)) {
        printf("\n--- %s ping statistics ---\n", inet_ntoa(*ip_2_ip4(&target_addr)));
    } else {
        printf("\n--- %s ping statistics ---\n", inet6_ntoa(*ip_2_ip6(&target_addr)));
    }
    printf("%d packets transmitted, %d received, %d%% packet loss, time %dms\n",
           transmitted, received, loss, total_time_ms);
    // delete the ping sessions, so that we clean up all resources and can create a new ping session
    // we don't have to call delete function in the callback, instead we can call delete function from other tasks
    /*
        need send a signal    
    */
    xEventGroupSetBits(s_wifi_event_group, WIFI_PING_SUCESS);

    esp_ping_delete_session(hdl);
}

static int do_ping_cmd(void)
{
    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    static esp_ping_handle_t ping;

    config.interval_ms = (uint32_t)(EXAMPLE_PING_INTERVAL * 1000);
    config.count = (uint32_t)(EXAMPLE_PING_COUNT);

    // parse IP address
    ip_addr_t target_addr;
    struct addrinfo hint;
    struct addrinfo *res = NULL;
    memset(&hint, 0, sizeof(hint));
    memset(&target_addr, 0, sizeof(target_addr));

    /* convert domain name to IP address */
    if (getaddrinfo(EXAMPLE_PING_IP, NULL, &hint, &res) != 0) {
        printf("ping: unknown host %s\n", EXAMPLE_PING_IP);
        return 1;
    }
    if (res->ai_family == AF_INET) {
        struct in_addr addr4 = ((struct sockaddr_in *) (res->ai_addr))->sin_addr;
        inet_addr_to_ip4addr(ip_2_ip4(&target_addr), &addr4);
    } else {
        struct in6_addr addr6 = ((struct sockaddr_in6 *) (res->ai_addr))->sin6_addr;
        inet6_addr_to_ip6addr(ip_2_ip6(&target_addr), &addr6);
    }
    freeaddrinfo(res);
    config.target_addr = target_addr;

    /* set callback functions */
    esp_ping_callbacks_t cbs = {
        .on_ping_success = cmd_ping_on_ping_success,
        .on_ping_timeout = cmd_ping_on_ping_timeout,
        .on_ping_end = cmd_ping_on_ping_end,
        .cb_args = NULL
    };

    esp_ping_new_session(&config, &cbs, &ping);
    esp_ping_start(ping);

    return 0;
}


static void modem_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id) {
    case ESP_MODEM_EVENT_PPP_START:
        ESP_LOGI(TAG, "Modem PPP Started");
        break;
    case ESP_MODEM_EVENT_PPP_STOP:
        ESP_LOGI(TAG, "Modem PPP Stopped");
        xEventGroupSetBits(event_group, STOP_BIT);
        break;
    case ESP_MODEM_EVENT_UNKNOWN:
        ESP_LOGW(TAG, "Unknown line received: %s", (char *)event_data);
        break;
    default:
        break;
    }
}

void process_config_data (type_of_mqtt_data_t data_type, mqtt_config_list mqtt_config_list , void* string_value, int int_value, bool bool_value)
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
            config_infor_now.charg_interval = int_value;
        }
        
        if (mqtt_config_list == UNCHARG_INTERVAL)
        {
            config_infor_now.uncharg_interval = int_value;
        }
        
        if (mqtt_config_list == SMOKE_WAKE)
        {
            config_infor_now.smokeSensorWakeupInterval = int_value;
        }
        
        if (mqtt_config_list == SMOKE_HEARTBEAT)
        {
            config_infor_now.smokeSensorHeartbeatInterval = int_value;
        }
        
        if (mqtt_config_list == SMOKE_THRESH)
        {
            config_infor_now.smokeSensorThresHole = int_value;
        }
        
        if (mqtt_config_list == TEMPER_WAKE)
        {
            config_infor_now.tempSensorWakeupInterval = int_value;
        }
        
        if (mqtt_config_list == TEMPER_HEARTBEAT)
        {
            config_infor_now.tempSensorHeartbeatInterval = int_value;
        }

        if (mqtt_config_list == TEMPER_THRESH)
        {
            config_infor_now.tempThresHold = int_value;
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



static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    char string_key[32];
    type_of_mqtt_data_t type_of_process_data = 0;
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        //publish info
        info_device_t device_info;
        memcpy (device_info.imei, GSM_IMEI, sizeof (GSM_IMEI));
        memcpy (device_info.simIMEI, SIM_IMEI, sizeof(SIM_IMEI));
        sprintf(device_info.firmware, "%s", FIRMWARE_VERSION);
        sprintf(device_info.hardwareVersion, "%s", HARDWARE_VERSION);
        make_device_info_payload (device_info, mqtt_payload);
        esp_mqtt_client_publish(mqtt_client, info_topic_header, (char*)mqtt_payload, 0, 0, 0);
        // truoc do can doc cau hinh ra tu flash
        ESP_LOGI (TAG, "READ CONFIG DATA FROM FLASH");
        // read_data_from_flash (&config_infor_now, sizeof (info_config_t), NVS_CONFIG_KEY);
        // send_current_config (config_infor_now);
        
        esp_err_t err = ESP_OK;
        for (mqtt_config_list i = 0; i < MAX_CONFIG_HANDLE; i++)
        {
            type_of_process_data = get_type_of_data (i);
            err = read_config_data_from_flash (&config_infor_now, type_of_process_data, i);
            if (err != ESP_OK)
            {
                make_key_to_store_type_in_nvs (string_key, i);
                ESP_LOGI (TAG, "GOT ERR %d WHILE READING %s KEY", err, string_key);
            }
        }
        send_current_config (config_infor_now);
        /*
            quet data trong flash va gui len server
        */
        // fire_status_t status_store_inflash;
        for (uint8_t i = 0; i < ping_data_count_in_flash; i++)
        {
            sprintf (ping_data_key, NVS_DATA_PING, i);
            // read_data_from_flash (&status_store_inflash, sizeof (fire_status_t), ping_data_key);
            internal_flash_nvs_read_string (ping_data_key, mqtt_payload, sizeof (mqtt_payload));
            // make_fire_status_payload (status_store_inflash, (char *)mqtt_payload); // Bo truong temper va fireZone
            esp_mqtt_client_publish (mqtt_client, heart_beat_topic_header, mqtt_payload, 0, 0, 0);
        }
        ping_data_count_in_flash = 0; // reset after send all data in flash
        // sensor_info_t data_sensor_store_in_flash;
        for (uint8_t i = 0; i < data_sensor_count_in_flash; i++)
        {
            sprintf (ping_data_key, NVS_DATA_SENSOR, i);
            //read_data_from_flash (&data_sensor_store_in_flash, sizeof (fire_status_t), ping_data_key);
            internal_flash_nvs_read_string (ping_data_key, mqtt_payload, sizeof (mqtt_payload));
            // make_sensor_info_payload (data_sensor_store_in_flash, (char *)mqtt_payload); // Bo truong temper va fireZone
            esp_mqtt_client_publish (mqtt_client, sensor_topic_header, mqtt_payload, 0, 0, 0);
        }
        data_sensor_count_in_flash = 0; // reset after send all data in flash
        xEventGroupSetBits(event_group, MQTT_CONNECT_BIT);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        xEventGroupSetBits(event_group, MQTT_DIS_CONNECT_BIT);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/esp-pppos", "esp32-pppos", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        xEventGroupSetBits(event_group, GOT_DATA_BIT);
        if (strstr (event->topic, "/update") && (strstr (event->data, "begin")))
        {
            xEventGroupSetBits(event_group, UPDATE_BIT);
            ESP_LOGI(TAG, "set bit update");
            //=>> can check bit
        }
        if ((strstr (event->topic, "/g2d/ota")))
        {
            xEventGroupSetBits(event_group, MQTT_DIS_CONNECT_BIT);
            ESP_LOGI(TAG, "set bit disconnect");
            if (strstr (event->data, "\"OTA\":0"))
            {
                //update esp32
                memset (ota_url, '\0', sizeof (ota_url));
                ota_server_str_buff = strstr (event->data, "\"url\":");
                ota_server_str_buff += strlen ("\"url\":");
                strtok (ota_server_str_buff, "\r\n");
                memcpy (ota_url, ota_server_str_buff, strlen (ota_server_str_buff));

                xEventGroupSetBits(event_group, UPDATE_BIT);
                ESP_LOGI(TAG, "set bit update");
            }
            else if (strstr (event->data, "\"OTA\":1"))
            {
                //update gd32
                // gửi bản tin min đến gd32 yêu cầu cập nhật, kết nối lên server sau đó gửi data qua
            }
            //=>> can test disconnect func  
        }

        if (strstr (event->topic, "/g2d/config/"))
        {
            cJSON* process_json = NULL;
            // cJSON* mqtt_add = NULL;
            // cJSON* mqtt_user = NULL;
            // cJSON* mqtt_pass = NULL;
            // cJSON* charg_interval = NULL;
            // cJSON* uncharg_interval = NULL;
            // cJSON* userPhoneNumber1 = NULL;
            // cJSON* userPhoneNumber2 = NULL;
            // cJSON* userPhoneNumber3 = NULL;
            // cJSON* networkAddress = NULL;
            // cJSON* buzzerEnable = NULL;
            // cJSON* syncAlarm = NULL;
            // cJSON* smokeSensorWakeupInterval = NULL;
            // cJSON* smokeSensorHeartbeatInterval = NULL;
            // cJSON* smokeSensorThresHole = NULL;
            // cJSON* tempSensorHeartbeatInterval = NULL;
            // cJSON* tempSensorWakeupInterval = NULL;
            // cJSON* tempSensorHeartbeatInterval = NULL;
            // cJSON* tempThresHold = NULL;
            cJSON* config_json = NULL;
            char* config = strstr (event->data, "{");
            config_json = parse_json (config);
            for (mqtt_config_list i = 0; i < MAX_CONFIG_HANDLE; i++)
            {
                process_json = NULL;
                type_of_process_data = get_type_of_data (i);
                make_key_from_mqtt_list_type (string_key, i);
                process_json = cJSON_GetObjectItemCaseSensitive (config_json, string_key);
                if (process_json == NULL)
                {
                    // if not found string
                    continue;
                }
                
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

            // memcpy (&config_infor_from_server, event->data, sizeof (info_config_from_server_t));
            // memcpy (config_infor_now.topic_hr, config_infor_from_server.topic_hr, sizeof (config_infor_now.topic_hr));
            // memcpy (config_infor_now.mqtt_add, config_infor_from_server.mqtt_add, sizeof (config_infor_now.topic_hr));
            // memcpy (config_infor_now.mqtt_user, config_infor_from_server.mqtt_user, sizeof (config_infor_now.topic_hr));
            // memcpy (config_infor_now.mqtt_pass, config_infor_from_server.mqtt_pass, sizeof (config_infor_now.topic_hr));
            // memcpy (config_infor_now.userPhoneNumber1, config_infor_from_server.userPhoneNumber1, sizeof (config_infor_now.topic_hr));
            // memcpy (config_infor_now.userPhoneNumber2, config_infor_from_server.userPhoneNumber2, sizeof (config_infor_now.topic_hr));
            // memcpy (config_infor_now.userPhoneNumber3, config_infor_from_server.userPhoneNumber3, sizeof (config_infor_now.topic_hr));
            // memcpy (config_infor_now.networkAddress, config_infor_from_server.networkAddress, sizeof (config_infor_now.topic_hr));
            // config_infor_now.charg_interval = config_infor_from_server.charg_interval;
            // config_infor_now.uncharg_interval = config_infor_from_server.uncharg_interval;
            // config_infor_now.buzzerEnable = config_infor_from_server.buzzerEnable;
            // config_infor_now.syncAlarm = config_infor_from_server.syncAlarm;
            // config_infor_now.smokeSensorWakeupInterval = config_infor_from_server.smokeSensorWakeupInterval;
            // config_infor_now.smokeSensorHeartbeatInterval = config_infor_from_server.smokeSensorHeartbeatInterval;
            // config_infor_now.smokeSensorThresHole = config_infor_from_server.smokeSensorThresHole;
            // config_infor_now.tempSensorHeartbeatInterval = config_infor_from_server.tempSensorHeartbeatInterval;
            // config_infor_now.tempSensorWakeupInterval = config_infor_from_server.smokeSensorWakeupInterval;
            // config_infor_now.tempThresHold = config_infor_from_server.tempThresHold;
            // lưu lại - can luu vao flash
            // esp_err_t err = write_data_to_flash (&config_infor_now, sizeof (info_config_t), NVS_CONFIG_KEY);
            // ESP_LOGI (TAG, "Write config data to flash: %s", (err = ESP_OK) ? "Fail" : "OK");
            // // doc ra tu flash
            // ESP_LOGI (TAG, "Read config data from flash now");
           // read_data_from_flash (&config_infor_now, sizeof (info_config_t), NVS_CONFIG_KEY);
            send_current_config (config_infor_now);
        }

        if (strstr (event->topic, "/bleConfigInfo"))/// CONFIG INFO GET FROM SERVER
        {
            
            ble_info_t ble_config_info;
            
            cJSON* netkey = NULL;
            cJSON* appkey = NULL;
            cJSON* iv_index = NULL;
            cJSON* sequence_number = NULL;
            cJSON* ble_config_json = NULL;

            char* config = strstr (event->data, "{");
            ble_config_json = parse_json (config);

            netkey = cJSON_GetObjectItemCaseSensitive (ble_config_json, "netkey");
            if (cJSON_IsString(netkey))
            {
                ESP_LOGI(TAG,"netkey:\"%s\"\n", netkey->valuestring);
                memcpy (ble_config_info.netkey, netkey->valuestring, strlen (netkey->valuestring));
            }
            appkey = cJSON_GetObjectItemCaseSensitive (ble_config_json, "appkey");

            if (cJSON_IsString(appkey))
            {
                ESP_LOGI(TAG,"appkey:\"%s\"\n", appkey->valuestring);
                memcpy (ble_config_info.appkey, netkey->valuestring, strlen (appkey->valuestring));
            }
           
            iv_index = cJSON_GetObjectItemCaseSensitive (ble_config_json, "iv_index");
            if (cJSON_IsNumber(iv_index))
            {
                ESP_LOGI(TAG,"iv_index:\"%d\"\n", iv_index->valueint);
                ble_config_info.iv_index = iv_index->valueint;
            }
            
            sequence_number = cJSON_GetObjectItemCaseSensitive (ble_config_json, "sequence_number");
            if (cJSON_IsNumber(sequence_number))
            {
                ESP_LOGI(TAG,"sequence_number:\"%d\"\n", sequence_number->valueint);
                ble_config_info.sequence_number = sequence_number->valueint;
            }
            if (ble_config_json != NULL)
            {
                cJSON_Delete(ble_config_json);
            }
            //set bit
            {
                min_msg_t ble_config_msg;
                ble_config_msg.id = MIN_ID_SEND_KEY_CONFIG;
                ble_config_msg.payload = &ble_config_info;
                ble_config_msg.len = sizeof (ble_config_info);
                send_min_data (&ble_config_msg);
            }
        }

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "MQTT other event id: %d", event->event_id);
        break;
    }
    return ESP_OK;
}

static void on_ppp_changed(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "PPP state changed event %d", (event_id - 256));
    if (event_id == NETIF_PPP_ERRORUSER) {
        /* User interrupted event from esp-netif */
        esp_netif_t *netif = *(esp_netif_t**)event_data;
        ESP_LOGI(TAG, "User interrupted event from netif:%p", netif);
    }
}


static void on_ip_event(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "IP event! %d", event_id);
    if (event_id == IP_EVENT_PPP_GOT_IP) {
        esp_netif_dns_info_t dns_info;

        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        esp_netif_t *netif = event->esp_netif;

        ESP_LOGI(TAG, "Modem Connect to PPP Server");
        ESP_LOGI(TAG, "~~~~~~~~~~~~~~");
        ESP_LOGI(TAG, "IP          : " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Netmask     : " IPSTR, IP2STR(&event->ip_info.netmask));
        ESP_LOGI(TAG, "Gateway     : " IPSTR, IP2STR(&event->ip_info.gw));
        esp_netif_get_dns_info(netif, 0, &dns_info);
        ESP_LOGI(TAG, "Name Server1: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
        esp_netif_get_dns_info(netif, 1, &dns_info);
        ESP_LOGI(TAG, "Name Server2: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
        ESP_LOGI(TAG, "~~~~~~~~~~~~~~");
        xEventGroupSetBits(event_group, CONNECT_BIT);
        ESP_LOGI(TAG, "GOT ip event!!!");
    } else if (event_id == IP_EVENT_PPP_LOST_IP) {
        ESP_LOGI(TAG, "Modem Disconnect from PPP Server");
    } else if (event_id == IP_EVENT_GOT_IP6) {
        ESP_LOGI(TAG, "GOT IPv6 event!");

        ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
        ESP_LOGI(TAG, "Got IPv6 address " IPV6STR, IPV62STR(event->ip6_info.ip));
    }
}

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;
    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
   // xEventGroupSetBits(event_group, CONNECT_BIT);
}


void gsm_gpio_config (void)
{
    //create zero io_config
    gpio_config_t io_config = {};
    io_config.pin_bit_mask = GSM_OUTPUT_SEL;
    io_config.mode = GPIO_MODE_OUTPUT;
    io_config.intr_type = GPIO_INTR_DISABLE;
    io_config.pull_down_en = 0;
    io_config.pull_up_en = 0;
    gpio_config (&io_config);
    io_config.pin_bit_mask = GSM_INPUT_SEL;
    io_config.mode = GPIO_MODE_INPUT;
    io_config.intr_type = GPIO_INTR_DISABLE;
    io_config.pull_down_en = 0;
    io_config.pull_up_en = 0;
    gpio_config (&io_config);
}

void send_current_config (info_config_t config_infor_now)
{
    sprintf (config_infor_now.httpDnsName, HTTP_SERVER);
    memset (config_infor_now.httpUsername, '\0', sizeof (config_infor_now.httpUsername));
    memset (config_infor_now.httpDnsPass, '\0', sizeof (config_infor_now.httpDnsPass));
    // sprintf (config_infor_now.httpDnsPort, WEB_PORT);
    config_infor_now.httpDnsPort = gsm_utilities_get_number_from_string (0, WEB_PORT);
    // dung sprintf
    sprintf (config_infor_now.wifiname, CONFIG_ESP_WIFI_SSID);
    sprintf (config_infor_now.wifipass, CONFIG_ESP_WIFI_PASSWORD);
    config_infor_now.wifiDisable = IS_WIFI_DISABLE;
    // config_infor_now.reset = RESET; //bo truong reset
    // dung sprintf
    sprintf (config_infor_now.pingMainServer, EXAMPLE_PING_IP);
    sprintf (config_infor_now.pingBackupServer, BACKUP_PING_IP);
    config_infor_now.inputActiveLevel = 1;
    // Smart module khong dung 3 truong nay
    config_infor_now.zoneMinMv = 0; //CONFIG_ZONE_MIN_MV;
    config_infor_now.zoneMaxMv = 0; //CONFIG_ZONE_MAX_MAX;
    config_infor_now.zoneDelay = 0; ///CONFIG_ZONE_DELAY;
    
    make_config_info_payload (config_infor_now, (char*)mqtt_payload);
    esp_mqtt_client_publish(mqtt_client, config_topic_header, (char*)mqtt_payload, 0, 0, 0);
}
//******************************************* NVS PLACE *****************************///
// void init_nvs_flash(void )
// {
//     esp_err_t err = nvs_flash_init();
//     if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         // NVS partition was truncated and needs to be erased
//         // Retry nvs_flash_init
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         err = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(err);
// }

// //  write data in flash
// int32_t write_data_to_flash(void *data_write, size_t byte_write, const char* key)
// {
//     nvs_handle_t my_handle;
//     esp_err_t ret = ESP_OK;
//     //OPEN FLASH
//     esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
//     ret |= err;

//     if (err != ESP_OK) {
//         ESP_LOGI(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
//         // ghi lại gia tri mac dinh và them version control
//     } else {
//         ESP_LOGI(TAG, "OPEN Done, Writing data to key %s\n", key);
//         err = nvs_set_blob (my_handle, key, data_write, byte_write);
//         ESP_LOGI(TAG, "%s", (err != ESP_OK) ? "Failed!" : "Done");
//         ret |= err;

//         ESP_LOGI(TAG, "\tCommitting updates string in NVS ... ");
//         err = nvs_commit(my_handle);
//         ESP_LOGI(TAG, "%s", (err != ESP_OK) ? "Failed!" : "Done");
//         ret |= err; 

//         nvs_close(my_handle);
//     }
//     return ret;
// }

// //read data blob from flash
// esp_err_t read_data_from_flash (void *data_read, uint16_t byte_read, const char* key)
// {
//     nvs_handle_t my_handle;
//     esp_err_t ret = ESP_OK;
//     esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
//     if (err != ESP_OK) {
//         ESP_LOGI(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
//         ret |= err;
//     } else 
//     {
//         esp_err_t err = nvs_get_blob(my_handle, key, data_read, (size_t*)&byte_read);
//         switch (err)
//         {
//         case ESP_OK:
//             break;

//         case ESP_ERR_NVS_NOT_FOUND:
//             ESP_LOGW(TAG, "Key %s not found", key);
//             ret |= err;
//             break;

//         default:
//             ESP_LOGE(TAG, "Error (%s) reading key %s!", esp_err_to_name(err), key);
//             ret |= err;
//             break;
//         }
//         nvs_close(my_handle);
//     }
//     //m_err_code = err;
//     return ret;
// }

//Callback for user tasks created in app_main()
void reset_task(void *arg)
{
    //Subscribe this task to TWDT, then check if it is subscribed
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
    ESP_ERROR_CHECK(esp_task_wdt_status(NULL));

    while(1){
        //reset the watchdog every 2 seconds
        ESP_ERROR_CHECK(esp_task_wdt_reset());  //Comment this line to trigger a TWDT timeout
        vTaskDelay(pdMS_TO_TICKS(TASK_RESET_PERIOD_S * 1000));
    }
}

void change_protocol_using_to (protocol_type protocol)
{
    if (protocol_using != protocol)
    {
        protocol_using = protocol;
    }
    else if (protocol_using == protocol)
    {
        ESP_LOGE (TAG, "WRONG PROTOCOL CHANGED, CHANGE AUTO");
        protocol_using = DEFAULT_PROTOCOL;
    }
}

// void handle_uart_data_task(void)
// {
//     uint8_t* data = (uint8_t*)malloc (10);
//     if (lwrb_read (&data_uart_module_rb, data, 10))
//     {
//         min_rx_feed(&m_min_context, data, 10);
//     }
//     free(data);
// }

uint32_t sys_get_ms(void)
{
    return (xTaskGetTickCount() / portTICK_RATE_MS);
}

esp_err_t err;

node_sensor_data_t node_data;

void min_rx_callback(void *min_context, min_msg_t *frame)
{
    switch (frame->id)
    {
    case MIN_ID_RECIEVE_SPI_FROM_GD32:
        {
            uint8_t data_rev_from_spi[256];
            memcpy (data_rev_from_spi, frame->payload, sizeof (data_rev_from_spi));
        }
        break;
    case MIN_ID_SEND_AND_RECEIVE_HEARTBEAT_MSG:
        // RECEIVE DATA NEED TO SEND TO MQTT SERVER
        memcpy (payload_buffer, frame->payload, 256);
        beacon_ping_data_handle = (app_beacon_ping_msg_t*) payload_buffer;
        // make payload to send 
        alarm_status.fire_status = beacon_ping_data_handle->alarm_value;
        alarm_status.csq = csq;
        alarm_status.sensor_cnt = beacon_ping_data_handle->sensor_count;
        alarm_status.battery = 0; /// NEED MONITOR BATTERRY
        alarm_status.updateTime = app_time ();
        switch (protocol_using)
        {
            case GSM_4G_PROTOCOL:
                sprintf (alarm_status.networkInterface, "GSM");
                break;
            case WIFI_PROTOCOL:
                sprintf (alarm_status.networkInterface, "WIFI");
                break;
            case ETHERNET_PROTOCOL:
                sprintf (alarm_status.networkInterface, "ETH");
                break;
            default:
                break;
        }
        // gsm status
        if (gsm_started)
        {
            alarm_status.networkStatus.value |= (1 << 8);
        }
        else
        {
            alarm_status.networkStatus.value &= ~(1 << 8);
        }
        //eth status
        if (eth_started)
        {
            alarm_status.networkStatus.value |= (1 << 7);
        }
        else
        {
            alarm_status.networkStatus.value &= ~(1 << 7);
        }
        // wifi status
        if (wifi_started)
        {
            alarm_status.networkStatus.value |= (1 << 6);
        }
        else
        {
            alarm_status.networkStatus.value &= ~(1 << 6);
        }
        make_fire_status_payload (alarm_status, (char *)mqtt_payload); // Bo truong temper va fireZone
        // sau khi co tao xong payload  thi kiem tra tinh trang ket noi mang va luu vao flash neu mat mang hoac ban len server sau khi co mang lai
        if ((alarm_status.networkStatus.value >> 5) == 0) // khong co mang haoc so sanh truc tiep tai bien nay
        {
            ping_data_count_in_flash++;
            sprintf (ping_data_key, NVS_DATA_PING, ping_data_count_in_flash);
            // err = write_data_to_flash (mqtt_payload, strlen ((char *)mqtt_payload), ping_data_key);
            err = internal_flash_nvs_write_string (ping_data_key, mqtt_payload);
            ESP_LOGI (TAG, "Write heartbeat data to flash: %s", (err = ESP_OK) ? "Fail" : "OK");
        }
        else
        {
            esp_mqtt_client_publish(mqtt_client, heart_beat_topic_header, (char*)mqtt_payload, 0, 0, 0);
        }
        break;
    case MIN_ID_SEND_AND_RECEIVE_BEACON_MSG:

        memcpy (payload_buffer, frame->payload, 256);
        beacon_data_handle = (app_beacon_data_t *) payload_buffer;
        memcpy(sensor_info.mac, beacon_data_handle->device_mac, 6);
        sprintf (sensor_info.firmware, "%d", beacon_data_handle->fw_verison);
        if (beacon_data_handle->propreties.Name.alarmState)
        {
            sensor_info.status = 1;
        }
        else
        {
            sensor_info.status = 0;
        }
        sensor_info.temperature = beacon_data_handle->teperature_value;
        sensor_info.smoke = beacon_data_handle->smoke_value;
        sensor_info.updateTime = app_time ();
        ESP_LOGI (TAG, "Sensor data event");
        make_sensor_info_payload (sensor_info, (char *)mqtt_payload); 
        if (!(gsm_started && wifi_started && eth_started))
        {
            data_sensor_count_in_flash++;
            sprintf (data_sensor_key, NVS_DATA_SENSOR, data_sensor_count_in_flash);
            // err = write_data_to_flash (mqtt_payload, strlen ((char *)mqtt_payload), data_sensor_key);
            err = internal_flash_nvs_write_string (ping_data_key, mqtt_payload);
            ESP_LOGI (TAG, "Write sensor data to flash: %s", (err = ESP_OK) ? "Fail" : "OK");
        }
        else 
        {
            esp_mqtt_client_publish(mqtt_client, sensor_topic_header, (char*)mqtt_payload, 0, 0, 0);//only send when network is starteed
        }
        break;
    case MIN_ID_PING_ESP_ALIVE:
        ESP_LOGI (TAG, "test ping gd32 ok, uart pass");
        break;
    case MIN_ID_PING_RESPONSE:
        ESP_LOGI (TAG, "ping gd 32 ok, uart pass");
        break;
    case MIN_ID_NEW_SENSOR_PAIRING:
        // luu vao flash de kiem soat
        memcpy (payload_buffer, frame->payload, 256);
        char mac_key [64];
        beacon_pair_info_t* pair_info;
        uint16_t unicast_addr;
        pair_info = (beacon_pair_info_t*) payload_buffer;
        {
            memcpy (node_data.device_mac, pair_info->device_mac, 6);
            node_data.device_type = pair_info->device_type;
            build_string_from_MAC (node_data.device_mac, mac_key);
            // write
            size_t len = sizeof (node_sensor_data_t);
            unicast_addr = find_and_write_into_mac_key_space(&node_data, &len, mac_key);
        }
        node_sensor_data_t sensor_data_response;
        memcpy (sensor_data_response.device_mac, pair_info->device_mac, 6);
        sensor_data_response.device_type = pair_info->device_type;
        sensor_data_response.unicast_add = unicast_addr;
        //send data through min
        min_msg_t min_pair_data_msg;
        min_pair_data_msg.id = MIN_ID_NEW_SENSOR_PAIRING;
        min_pair_data_msg.payload = &sensor_data_response;
        min_pair_data_msg.len = sizeof (beacon_pair_info_t);
        send_min_data (&min_pair_data_msg);
        break;
    default:
        break;
    }
}

bool min_tx_byte(void *ctx, uint8_t byte)
{
	(void)ctx;
	uart_write_bytes(UART_NUM_1, &byte, 1);
	return true;
}

void send_min_data(min_msg_t *min_msg)
{
	min_send_frame(&m_min_context, min_msg);
    if (min_msg->id == MIN_ID_SEND_KEY_CONFIG)
    {
        ESP_LOGI (TAG, "send key config");    
    }
    ESP_LOGI (TAG, "send min msg ID: %d", min_msg->id);
}

void build_min_tx_data_for_spi(min_msg_t* min_msg, uint8_t* data_spi, uint8_t size)
{
    min_msg->id = MIN_ID_SEND_SPI_FROM_ESP32;
    memcpy (min_msg->payload, data_spi, size);
    min_msg->len = size;
}

// void deinit_interface (protocol_type protocol)
// {
//     switch (protocol)
//     {
//     case WIFI_PROTOCOL:
//             esp_mqtt_client_destroy(mqtt_client);
//             ESP_LOGI(TAG, "destroy mqtt");
//             esp_wifi_stop();
//             wifi_started = false;
//         break;
//     case ETHERNET_PROTOCOL:
//             esp_mqtt_client_destroy(mqtt_client);
//             ESP_ERROR_CHECK(esp_eth_stop(eth_handle));
//             eth_started = false; 
//             ESP_LOGI(TAG, "destroy mqtt");
//         break;
//     case GSM_4G_PROTOCOL:
//             esp_mqtt_client_destroy(mqtt_client);
//             ESP_LOGI(TAG, "destroy mqtt");
//             /* Exit PPP mode */
//             ESP_ERROR_CHECK(esp_modem_stop_ppp(dte));
//             gsm_started = false;
//             ESP_LOGI(TAG, "ppp stop");
//             xEventGroupWaitBits(event_group, STOP_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
// #if CONFIG_EXAMPLE_SEND_MSG
//             const char *message = "Welcome to ESP32!";
//             ESP_ERROR_CHECK(example_send_message_text(dce, CONFIG_EXAMPLE_SEND_MSG_PEER_PHONE_NUMBER, message));
//             ESP_LOGI(TAG, "Send send message [%s] ok", message);
// #endif
//             /* Power down module */
//             if (dce != NULL)
//             {
//                 ESP_ERROR_CHECK(dce->power_down(dce));
//                 ESP_LOGI(TAG, "Power down");
//                 ESP_ERROR_CHECK(dce->deinit(dce)); //deinit
//             }
//             if (esp_modem_netif_clear_default_handlers(modem_netif_adapter) == ESP_OK)
//             {
//                 ESP_LOGI ("UNREGISTER", "CLEAR TO REINIT");
//             }
//             esp_modem_netif_teardown(modem_netif_adapter);
//             esp_netif_destroy(esp_netif);
//             ESP_LOGI ("UNREGISTER", "CLEAR TO REINIT");
//         break;
//     default:
//         ESP_LOGE (TAG, "UNHANDLABLE PROTOCOL DEINIT");
//         break;
//     }
// }

// app cli function
void cli_cdc_tx(uint8_t *buffer, uint32_t size)
{
	uart_write_bytes(UART_NUM_0 ,buffer, size);
}

int cli_cdc_puts(const char *msg)
{
	uint32_t len = strlen(msg);
    uart_write_bytes(UART_NUM_0 ,msg, len);
	return len;
}

// void handle_mutinetif()
// {
//     esp_netif_t *netifs[3]; // there are 3 netif to handle
//     netifs[0] = gsm_esp_netif;
//     netifs[1] = wifi_netif;
//     netifs[2] = eth_netif;

//     for (int i; i < 3; i++)
//     {
//         esp_netif_action_start(netifs[i], 0, 0, 0);
//         esp_netif_action_connected(netifs[i], 0, 0, 0);
//     }
//     assert(esp_netif_get_netif_impl(netifs[0]), netif_default);

// }

typedef enum 
{
    GSM_NETIF,
    ETH_NETIF,
    WIFI_NETIF, 
    NO_NETIF
} network_netif;

static bool is_our_netif(const char *prefix, esp_netif_t *netif)
{
    return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1) == 0;
}

esp_err_t get_interface_status (void)
{
    esp_netif_t *netif = NULL;
    esp_netif_ip_info_t ip;

    for (int i = 0; i < esp_netif_get_nr_of_ifs(); ++i) {
        netif = esp_netif_next(netif);
        if (is_our_netif(TAG, netif)) {
            ESP_LOGI(TAG, "Now connected to %s", esp_netif_get_desc(netif));
            ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip));
            ESP_LOGI(TAG, "- IPv4 address: " IPSTR, IP2STR(&ip.ip));
        }
    }
    
    return ESP_OK;
}

static network_netif check_and_update_default_netif (void)
{
    network_netif netif_now;
    if (gsm_started)
    {
        netif_now = GSM_NETIF;
    }
    else if (eth_started)
    {
        netif_now = ETH_NETIF;
    }
    else if (wifi_started)
    {
        netif_now = WIFI_NETIF;
    }
    else
    {
        netif_now = NO_NETIF;
    }
    return netif_now;
}



void app_main(void)
{
#warning "need gpio config"
    gsm_gpio_config ();


#ifdef ESP32_S2
    // USB
    tinyusb_config_t tusb_cfg = { 0 }; // the configuration uses default values
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t amc_cfg = { 0 }; // the configuration uses default values
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&amc_cfg));  
    
    ESP_LOGI(TAG, "USB initialization DONE");
    // USB end
#endif
    //uart begin
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
        };
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, BUF_SIZE * 2, 512, &uart1_queue, 0);
    uart_param_config(UART_NUM_1, &uart_config);        
    uart_set_pin(UART_NUM_1, GPIO_TX1, GPIO_RX1, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);//uart for connectivity from esp to gd
    // need one more uart for rs485
    //ESP32 <==> 4G MODULE (in ec2x_init)
   
    lwrb_init (&data_uart_module_rb, uart_buffer, UART_RINGBUFF_SIZE); //init lwrb
    m_min_setting.get_ms = sys_get_ms;
    m_min_setting.last_rx_time = 0x00;
	m_min_setting.rx_callback = min_rx_callback;
	m_min_setting.timeout_not_seen_rx = 5000;
	m_min_setting.tx_byte = min_tx_byte;
	m_min_setting.use_timeout_method = 1;
    m_min_context.callback = &m_min_setting;
	m_min_context.rx_frame_payload_buf = min_rx_buffer;
	min_init_context(&m_min_context);
#if CONFIG_LWIP_PPP_PAP_SUPPORT
    esp_netif_auth_type_t auth_type = NETIF_PPP_AUTHTYPE_PAP;
#elif CONFIG_LWIP_PPP_CHAP_SUPPORT
    esp_netif_auth_type_t auth_type = NETIF_PPP_AUTHTYPE_CHAP;
#elif !defined(CONFIG_EXAMPLE_MODEM_PPP_AUTH_NONE)
#error "Unsupported AUTH Negotiation"
#endif
    // create event loop and register callback
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &on_ip_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(NETIF_PPP_STATUS, ESP_EVENT_ANY_ID, &on_ppp_changed, NULL));
    //creat share resources 
    event_group = xEventGroupCreate();
    GSM_Sem = xSemaphoreCreateMutex();
    mqtt_info_queue = xQueueCreate(1, sizeof(mqtt_info_struct));
    
     xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);
    // after this we can test app cli
    
    
    //wdt init
    //ESP_ERROR_CHECK(esp_task_wdt_init(TWDT_TIMEOUT_S, false));
    //subcribe this task and checks it
    //ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
    //ESP_ERROR_CHECK(esp_task_wdt_status(NULL));
    init_nvs_flash ();
    // while (1)
    // {
    //     ESP_ERROR_CHECK(esp_task_wdt_reset());
    //     static uint32_t count =0;
    //     count++;
    //     if (count > 30)
    //     {
    //         send_min_data ((min_msg_t*) &ping_min_msg_nonsen);
    //         ESP_LOGI (TAG, "prepare to reset now");
           
    //     }
    //     else
    //     {
    //         send_min_data ((min_msg_t*) &ping_min_msg);
    //         ESP_LOGI (TAG, "Send ping requesst");
    //     }
        
    //     vTaskDelay(100 / portTICK_PERIOD_MS);
    // }

    /* create dte object */
    esp_modem_dte_config_t dte_config = ESP_MODEM_DTE_DEFAULT_CONFIG();
    /* setup UART specific configuration based on kconfig options */

    dte_config.tx_io_num = GPIO_TX0;
    dte_config.rx_io_num = GPIO_RX0;
    dte_config.rts_io_num = 0;
    dte_config.cts_io_num = 0;
    dte_config.rx_buffer_size = CONFIG_EXAMPLE_MODEM_UART_RX_BUFFER_SIZE;
    dte_config.tx_buffer_size = CONFIG_EXAMPLE_MODEM_UART_TX_BUFFER_SIZE;
    dte_config.event_queue_size = CONFIG_EXAMPLE_MODEM_UART_EVENT_QUEUE_SIZE;
    dte_config.event_task_stack_size = CONFIG_EXAMPLE_MODEM_UART_EVENT_TASK_STACK_SIZE;
    dte_config.event_task_priority = CONFIG_EXAMPLE_MODEM_UART_EVENT_TASK_PRIORITY;
    dte_config.dte_buffer_size = CONFIG_EXAMPLE_MODEM_UART_RX_BUFFER_SIZE / 2;
    
    dte = esp_modem_dte_init(&dte_config);
    if (dte == NULL)
    {
        ESP_LOGI (TAG, "DTE INIT FAIL");
    }
    while (1)
    {
        if (m_cli_started == false)
        {
            m_cli_started = true;
            app_cli_start(&m_tcp_cli);
            ESP_LOGI(TAG, "APP CLI STARTED \r\n");
        }
        uint8_t ch = '\0';
        uart_read_bytes (UART_NUM_0, &ch, 1, 100);
        if (ch)
        {
            app_cli_poll(ch);
        }
        // ESP_LOGI(TAG, "WAIT HERE TO TEST COMMAND LINE");
        // send_min_data ((min_msg_t*) &ping_min_msg);
        vTaskDelay (1/portTICK_RATE_MS);
    }
/*  uncomment this if there are no need gsm_task
    // Init netif object
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_PPP();
    esp_netif_t *esp_netif = esp_netif_new(&cfg);
    assert(esp_netif);
    void *modem_netif_adapter;
*/
    
    if(dce == NULL)
    dce = ec2x_init (dte);
    xSemaphoreTake (GSM_Sem, 200000);
    // GSM_IMEI[16]; store gsm imei
    if(dce == NULL)
    {
        // INIT FAIL
        protocol_using = WIFI_PROTOCOL;
    }
    
    EventBits_t ubits;

    // ETHERNET INIT Emac
    
    //eth netif config
    esp_netif_ip_info_t ip_info;
    esp_netif_inherent_config_t netif_eth_config = {
        .flags = ESP_NETIF_FLAG_AUTOUP,
        .ip_info = (esp_netif_ip_info_t*)&ip_info,
        .if_key = "eth",
        .if_desc = "net_eth_if",
        .route_prio = 3
    };
    esp_netif_config_t cfg = {
        .base = &netif_eth_config,                 // use specific behaviour configuration
        .driver = NULL,
        .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH, // use default WIFI-like network stack configuration
    };

    eth_netif = esp_netif_new(&cfg);

    // Init MAC and PHY configs to default
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    // reinit gpio
    phy_config.reset_gpio_num = CONFIG_ETH_PHY_RST_GPIO;
    mac_config.smi_mdc_gpio_num = CONFIG_ETH_MDC_GPIO;
    mac_config.smi_mdio_gpio_num = CONFIG_ETH_MDIO_GPIO;

    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_ip101(&phy_config);
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
     
    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));
    /* attach Ethernet driver to TCP/IP stack */
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));
    ESP_LOGI (TAG, "netif attach done");
    // end ethernet

    /* Register event handler */
    if (dce != NULL)
    {
        ESP_ERROR_CHECK(esp_modem_set_event_handler(dte, modem_event_handler, ESP_EVENT_ANY_ID, NULL)); //FOR MODEM
    }
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL)); //  FOR ETH
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL)); // FOR IP
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_PPP_GOT_IP, &got_ip_event_handler, NULL)); // FOR IP

    ESP_LOGI (TAG, "BEGIN WIFI");
    app_wifi_connect (wifi_name, wifi_pass);
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           5000);
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
        wifi_started = true;
        protocol_using = WIFI_PROTOCOL;
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
        wifi_started = false;
        protocol_using = ETHERNET_PROTOCOL;
    }
    else
    {
        wifi_started = false;
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    ESP_LOGI (TAG, "WIFI CONNECT");
    if (wifi_started == false)
    {
        ESP_ERROR_CHECK(esp_eth_start (eth_handle)); //start ethenet 
        ESP_LOGI (TAG, "ETH CONNECT");
        eth_started = true;
    }

    do_ping_cmd();//pinging to addr
    ESP_ERROR_CHECK(esp_task_wdt_reset());
    //init testing applications
    EventBits_t uxBitsPing = xEventGroupWaitBits(s_wifi_event_group, WIFI_PING_TIMEOUT | WIFI_PING_SUCESS, pdTRUE, pdFALSE, portMAX_DELAY);
    if (uxBitsPing & WIFI_PING_TIMEOUT)
    {
        ESP_LOGI (TAG, "PING TIMEOUT");
        protocol_using = ETHERNET_PROTOCOL;
    }
    else if (uxBitsPing & WIFI_PING_SUCESS)
    {
        ESP_LOGI (TAG, "PING OK");
    }
    else
    {
        ESP_LOGI (TAG, "PING UNEXPECTED EVENT");
    }
    
    mqtt_info_struct mqtt_broker_str;
    ESP_ERROR_CHECK(esp_task_wdt_reset());
    if (mqtt_server_ready == false)
    {
        // get mqtt server from http request
        xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);

        BaseType_t res = xQueueReceive (mqtt_info_queue, &mqtt_broker_str, 20000/ portTICK_RATE_MS);
        if (res == pdTRUE)
        {
            ESP_LOGI (TAG, "Recieve queue\r\n");
        }
        else if (res == pdFALSE)
        {
            ESP_LOGI (TAG, "Recieve queue fail\r\n");
            esp_restart();
        }
        //parse_mqtt_info (mqtt_broker_str, mqtt_host, &mqtt_port, mqtt_username, mqtt_password);
        
        // sprintf (mqtt_host , "\"%s\"", mqtt_broker_str.server_ip);
        // mqtt_port = mqtt_broker_str.port;
        // sprintf (mqtt_username , "\"%s\"", mqtt_broker_str.username);
        // sprintf (mqtt_password , "\"%s\"", mqtt_broker_str.password);

        memcpy (mqtt_host, mqtt_broker_str.server_ip, strlen (mqtt_broker_str.server_ip));
        mqtt_port = mqtt_broker_str.port;
        memcpy (mqtt_username, mqtt_broker_str.username, strlen (mqtt_broker_str.username));
        memcpy (mqtt_password, mqtt_broker_str.password, strlen (mqtt_broker_str.password));
        
        ESP_LOGI (TAG,"queue form host:%s\r\n", mqtt_host);
        ESP_LOGI (TAG,"queue form port:%d\r\n", mqtt_port);
        ESP_LOGI (TAG,"queue form username:%s\r\n", mqtt_username);
        ESP_LOGI (TAG,"queue form password:%s\r\n", mqtt_password);
        /* Config MQTT */
        mqtt_config.host = mqtt_host;
        mqtt_config.port = mqtt_port;
        mqtt_config.username = mqtt_username;
        mqtt_config.password = mqtt_password;
        mqtt_config.event_handle = mqtt_event_handler;
        mqtt_server_ready = true;
    }

    static uint32_t now;
    static uint32_t last_tick_cnt = 0;
    
    
    while(1)
    {
        send_min_data ((min_msg_t*) &ping_min_msg);
        get_interface_status ();
        ESP_ERROR_CHECK(esp_task_wdt_reset());
        ESP_LOGI(TAG, "PROTOCOL USE: %d ", protocol_using);

        // if (protocol_using != WIFI_PROTOCOL)
        // {
        //     app_wifi_connect (wifi_name, wifi_pass);
        //     EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
        //                                    WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        //                                    pdFALSE,
        //                                    pdFALSE,
        //                                    5000);
        //     if (bits & WIFI_CONNECTED_BIT)
        //     {
        //         ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
        //                 CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
        //         wifi_started = true;
        //         protocol_using = WIFI_PROTOCOL;
        //     }
        //     else if (bits & WIFI_FAIL_BIT)
        //     {
        //         ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
        //                 CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
        //         protocol_using = ETHERNET_PROTOCOL;
        //     }
        //     else
        //     {
        //         ESP_LOGE(TAG, "UNEXPECTED EVENT");
        //     }
        // }
        // else if ((protocol_using != ETHERNET_PROTOCOL) && (protocol_using != WIFI_PROTOCOL))
        // {
        //     protocol_using = ETHERNET_PROTOCOL;
        // }
        network_netif nn = check_and_update_default_netif ();
        if (nn == GSM_NETIF)
        {
            protocol_using = GSM_4G_PROTOCOL;
        }
        else if (nn == ETH_NETIF)
        {
            protocol_using = ETHERNET_PROTOCOL;
        }
        else if (nn == WIFI_NETIF)
        {
            protocol_using = WIFI_PROTOCOL;
        }
        else
        {
            protocol_using = PROTOCOL_NONE;
        }
        switch (protocol_using)
        {
        case WIFI_PROTOCOL:
            // if (wifi_started == false)
            // {
            //     ESP_LOGI (TAG, "BEGIN WIFI");
            //     app_wifi_connect (wifi_name, wifi_pass);
            //     ESP_LOGI (TAG, "WIFI CONNECT");
            //     do_ping_cmd(); //pinging to addr 
            //     EventBits_t uxBitsPing = xEventGroupWaitBits(s_wifi_event_group, WIFI_PING_TIMEOUT | WIFI_PING_SUCESS, pdTRUE, pdFALSE, portMAX_DELAY);
            //     if (uxBitsPing & WIFI_PING_TIMEOUT)
            //     {
            //         ESP_LOGI (TAG, "PING TIMEOUT, RETRY WITH ETH");
            //         change_protocol_using_to (ETHERNET_PROTOCOL);
            //         continue;
            //     }
            //     else if (uxBitsPing & WIFI_PING_SUCESS)
            //     {
            //         ESP_LOGI (TAG, "PING OK");
                    wifi_started = true;
            //     }
            //     else
            //     {
            //         ESP_LOGI (TAG, "PING UNEXPECTED EVENT");
            //     }
            // }
            break;
        case ETHERNET_PROTOCOL:
            // if (eth_started == false)
            // {
            //     ESP_ERROR_CHECK(esp_eth_start(eth_handle));
            eth_started = true;
            // }
        break;
        case GSM_4G_PROTOCOL:
            gsm_started = true;
            // dce = NULL;
            // dce = ec2x_init (dte);

            // if(dce == NULL)
            // {
            //     ESP_LOGE(TAG, "INT 4G FAIL");
            //     //protocol_using = WIFI_PROTOCOL;
            //     change_protocol_using_to (WIFI_PROTOCOL);
            //     continue;
            // }
            // xEventGroupWaitBits(event_group, CONNECT_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
        break;
        default:
            eth_started = false;
            gsm_started = false;
            wifi_started = false;
            ESP_LOGE(TAG, "No internet connected");
            break;
        }
        // wait connected then we listen to mqtt sever
        if (mqtt_server_ready == false)
        {
            ESP_LOGI (TAG, "MQTT CONFIG");
            // get mqtt server from http request
            xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);

            BaseType_t res = xQueueReceive (mqtt_info_queue, &mqtt_broker_str, 20000/ portTICK_RATE_MS);
            if (res == pdTRUE)
            {
                ESP_LOGD (TAG, "Recieve queue\r\n");
            }
            else if (res == pdFALSE)
            {
                ESP_LOGI (TAG, "Recieve queue fail\r\n");
                esp_restart();
            }
            //parse_mqtt_info (mqtt_broker_str, mqtt_host, &mqtt_port, mqtt_username, mqtt_password);
            
            memcpy (mqtt_host, mqtt_broker_str.server_ip, strlen (mqtt_broker_str.server_ip));
            mqtt_port = mqtt_broker_str.port;
            memcpy (mqtt_username, mqtt_broker_str.username, strlen (mqtt_broker_str.username));
            memcpy (mqtt_password, mqtt_broker_str.password, strlen (mqtt_broker_str.password));
            
            ESP_LOGI (TAG,"queue form host:%s\r\n", mqtt_host);
            ESP_LOGI (TAG,"queue form port:%d\r\n", mqtt_port);
            ESP_LOGI (TAG,"queue form username:%s\r\n", mqtt_username);
            ESP_LOGI (TAG,"queue form password:%s\r\n", mqtt_password);

            /* Config MQTT */
            mqtt_config.host = mqtt_host;
            mqtt_config.port = mqtt_port;
            mqtt_config.username = mqtt_username;
            mqtt_config.password = mqtt_password;
            mqtt_config.event_handle = mqtt_event_handler;
            mqtt_server_ready = true;
        
            mqtt_client = esp_mqtt_client_init(&mqtt_config);
            ESP_LOGI (TAG, "MQTT INIT");
            esp_mqtt_client_start(mqtt_client);
        }
        
        // we modulized connect event
        mqtt_client = esp_mqtt_client_init(&mqtt_config);
        ESP_LOGI (TAG, "MQTT INIT");
        esp_mqtt_client_start(mqtt_client);
        //register topic
        if (memcmp (GSM_IMEI, "not init yet", 16) != 0)
        {
            
            make_mqtt_topic_header (HEART_BEAT_HEADER, "smart_module", GSM_IMEI, heart_beat_topic_header);
            int msg_id = esp_mqtt_client_subscribe(mqtt_client, heart_beat_topic_header, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            make_mqtt_topic_header (FIRE_ALARM_HEADER, "smart_module", GSM_IMEI, fire_alarm_topic_header);
            msg_id = esp_mqtt_client_subscribe(mqtt_client, fire_alarm_topic_header, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            
            make_mqtt_topic_header (SENSOR_HEADER, "smart_module", GSM_IMEI, sensor_topic_header);
            msg_id = esp_mqtt_client_subscribe(mqtt_client, sensor_topic_header, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            make_mqtt_topic_header (INFO_HEADER, "smart_module", GSM_IMEI, info_topic_header);
            msg_id = esp_mqtt_client_subscribe(mqtt_client, info_topic_header, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            make_mqtt_topic_header (CONFIG_HEADER, "smart_module", GSM_IMEI, config_topic_header);
            msg_id = esp_mqtt_client_subscribe(mqtt_client, config_topic_header, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);        

            make_mqtt_topic_header (OTA_HEADER, "smart_module", GSM_IMEI, ota_header);
            msg_id = esp_mqtt_client_subscribe(mqtt_client, ota_header, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);           
        }
        while (1) //main process loop
        {
            ESP_ERROR_CHECK(esp_task_wdt_reset());
            EventBits_t uxBits = xEventGroupWaitBits(event_group, MQTT_DIS_CONNECT_BIT, pdTRUE, pdTRUE, 5/ portTICK_RATE_MS); // piority check disconnected event
            if ( (uxBits & MQTT_DIS_CONNECT_BIT) == MQTT_DIS_CONNECT_BIT)
            {
                ESP_LOGI (TAG, "mqtt disconnected bitrev");
                vTaskDelay(5 / portTICK_PERIOD_MS);
                //check if there are any interface to save data in flash
                mqtt_server_ready = false;
                break;
            }
            uint32_t timestamp_now = app_time();
            //need send it to gd32
            static esp_status_infor_t  esp_status_infor;
            min_msg_t syn_time_and_status_msg;
            if (timestamp_now)
            {
                // if get time update send it to gd 32
                // check network status
                esp_status_infor.need_update_time = true;
                if (wifi_started)
                {
                    esp_status_infor.network_status = WIFI_CONNECT;
                }
                else if (eth_started)
                {
                    esp_status_infor.network_status = ETH_CONNECT;
                }
                else if (gsm_started)
                {
                    esp_status_infor.network_status = GSM_CONNECT;
                }
                else
                {
                    esp_status_infor.network_status = NO_CONNECT;
                }
                esp_status_infor.timestamp_count_by_second = timestamp_now;
                syn_time_and_status_msg.id = MIN_ID_TIMESTAMP;
                syn_time_and_status_msg.payload = &esp_status_infor;
                syn_time_and_status_msg.len = sizeof (uint32_t);
                send_min_data (&syn_time_and_status_msg);
            }
            else
            {
                // no need send timestamp just network status
                esp_status_infor.need_update_time = false;
                if (wifi_started)
                {
                    esp_status_infor.network_status = WIFI_CONNECT;
                }
                else if (eth_started)
                {
                    esp_status_infor.network_status = ETH_CONNECT;
                }
                else if (gsm_started)
                {
                    esp_status_infor.network_status = GSM_CONNECT;
                }
                else
                {
                    esp_status_infor.network_status = NO_CONNECT;
                }
                esp_status_infor.timestamp_count_by_second = 0;
                syn_time_and_status_msg.id = MIN_ID_TIMESTAMP;
                syn_time_and_status_msg.payload = &esp_status_infor;
                syn_time_and_status_msg.len = sizeof (uint32_t);
                send_min_data (&syn_time_and_status_msg);
            }

            //xEventGroupWaitBits(event_group, GOT_DATA_BIT, pdTRUE, pdTRUE, 1000 / portTICK_RATE_MS);
            ubits = xEventGroupWaitBits (event_group, UPDATE_BIT, pdTRUE, pdTRUE, 5/ portTICK_RATE_MS); 
            if (ubits & UPDATE_BIT)
            {
                xTaskCreate(&advanced_ota_example_task, "advanced_ota_example_task", 1024 * 8, NULL, 5, NULL);
            }

            /* this space for functions */
            now = sys_get_ms();
            if ((now - last_tick_cnt) > 1000)
            {
                // ping gd32 while being alive
                send_min_data ((min_msg_t*) &ping_min_msg);
                last_tick_cnt = now;
                //example 
                /*
                    char str_header [64];
                    make_mqtt_topic_header (HEART_BEAT_HEADER, "bytech", GSM_IMEI, str_header);
                    fire_status_t fire_info_heartbeat;
                    char str_payload[256];
                    make_fire_status_payload (&fire_info_heartbeat, str_payload);
                    int msg_id = esp_mqtt_client_publish(mqtt_client, str_header, str_payload, 0, 0, 0);
                */
            }
            /*
                lấy thông tin từ mqtt gửi xuống ble
                định kì nhận bản tin heartbeat gửi lên topic
                lấy đc IMEI thiết bị check sim các kiểu
                xử lí trường hợp server chết
            */
        }
        //deinit after get out of loop to reinit in new loop


        //deinit_interface (protocol_using);
    }
//    ESP_ERROR_CHECK(dte->deinit(dte));
}


