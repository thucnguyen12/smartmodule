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
#include "sdkconfig.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"
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
// #include "protocol_examples_common.h"
//tiny usb (for esp32-s2 only)
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
//RTC
#include "rtc.h"
#include "esp_sntp.h"


#define BROKER_URL "mqtt://mqtt.eclipseprojects.io:1883"
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#define CONFIG_ESP_WIFI_SSID "BYTECH_T3"
#define CONFIG_ESP_WIFI_PASSWORD "bytech@2020"
#define DEFAULT_PROTOCOL WIFI_PROTOCOL

#define EXAMPLE_PING_IP            "www.google.com"
#define EXAMPLE_PING_COUNT         5
#define EXAMPLE_PING_INTERVAL      1

#define CONFIG_EXAMPLE_SKIP_VERSION_CHECK 1

static const char *TAG = "pppos_example";
static EventGroupHandle_t event_group = NULL;
static const int CONNECT_BIT = BIT0;
static const int STOP_BIT = BIT1;
static const int GOT_DATA_BIT = BIT2;
static const int MQTT_DIS_CONNECT_BIT = BIT3;
static const int WAIT_BIT = BIT4; 
TaskHandle_t m_uart_task = NULL;
SemaphoreHandle_t GSM_Sem;
SemaphoreHandle_t GSM_Task_tearout;
extern esp_netif_t *esp_netif;
extern void *modem_netif_adapter;
extern EventGroupHandle_t s_wifi_event_group;
modem_dte_t *dte = NULL;
// static bool network_connected = false;
// extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
// extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");


typedef enum
{
    GSM_4G_PROTOCOL,
    WIFI_PROTOCOL
} protocol_type;
protocol_type protocol_using = DEFAULT_PROTOCOL;

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

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, "/topic/esp-pppos", 0);
        msg_id = esp_mqtt_client_subscribe(client, "/update", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
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
            xEventGroupSetBits(event_group, WAIT_BIT);
            ESP_LOGI(TAG, "set bit update");
            //=>> can check bit
        }
        if ((strstr (event->topic, "/update")) && strstr (event->data, "test exit"))
        {
            xEventGroupSetBits(event_group, MQTT_DIS_CONNECT_BIT);
            ESP_LOGI(TAG, "set bit disconnect");
            //=>> can check bit  
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
}

static esp_err_t validate_image_header(esp_app_desc_t *new_app_info)
{
    if (new_app_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }

#ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
    if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0) {
        ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
        return ESP_FAIL;
    }
#endif

#ifdef CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
    /**
     * Secure version check from firmware image header prevents subsequent download and flash write of
     * entire firmware image. However this is optional because it is also taken care in API
     * esp_https_ota_finish at the end of OTA update procedure.
     */
    const uint32_t hw_sec_version = esp_efuse_read_secure_version();
    if (new_app_info->secure_version < hw_sec_version) {
        ESP_LOGW(TAG, "New firmware security version is less than eFuse programmed, %d < %d", new_app_info->secure_version, hw_sec_version);
        return ESP_FAIL;
    }
#endif

    return ESP_OK;
}

static esp_err_t _http_client_init_cb(esp_http_client_handle_t http_client)
{
    esp_err_t err = ESP_OK;
    /* Uncomment to add custom headers to HTTP request */
    // err = esp_http_client_set_header(http_client, "Custom-Header", "Value");
    return err;
}
void time_sync_notification_cb(struct timeval *tv)
{

    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    sntp_init();
}

static void obtain_time(void)
{
    /* this is not necessory cause we alreadey connected

    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    
    */

    /**
     * NTP server address could be aquired via DHCP,
     * see LWIP_DHCP_GET_NTP_SRV menuconfig option
     */

#ifdef LWIP_DHCP_GET_NTP_SRV
    sntp_servermode_dhcp(1);
#endif

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    // ESP_ERROR_CHECK(example_connect());
    
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);

    //ESP_ERROR_CHECK( example_disconnect() );
}

void app_time(void)
{
    time_t now;
    struct tm timeinfo;
    // need update time before get localtime
    time(&now);
    localtime_r(&now, &timeinfo);
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
    setenv("TZ", "Etc/GMT+7", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in New York is: %s", strftime_buf);
}

void advanced_ota_example_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Starting Advanced OTA example");

    esp_err_t ota_finish_err = ESP_OK;
    esp_http_client_config_t config = {
        .url = "http://192.168.2.241/pppos_client.bin",
        .cert_pem = (char *)"",
        .timeout_ms = 5000,
        .keep_alive_enable = true,
    };

#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL_FROM_STDIN
    char url_buf[OTA_URL_SIZE];
    if (strcmp(config.url, "FROM_STDIN") == 0) {
        example_configure_stdin_stdout();
        fgets(url_buf, OTA_URL_SIZE, stdin);
        int len = strlen(url_buf);
        url_buf[len - 1] = '\0';
        config.url = url_buf;
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong firmware upgrade image url");
        abort();
    }
#endif

#ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
#endif

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
        .http_client_init_cb = _http_client_init_cb, // Register a callback to be invoked after esp_http_client is initialized
    };

    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed");
        vTaskDelete(NULL);
    }

    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_read_img_desc failed");
        goto ota_end;
    }
    err = validate_image_header(&app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "image header verification failed");
        goto ota_end;
    }

    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
        // esp_https_ota_perform returns after every read operation which gives user the ability to
        // monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
        // data read so far.
        ESP_LOGD(TAG, "Image bytes read: %d", esp_https_ota_get_image_len_read(https_ota_handle));
    }

    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        // the OTA image was not completely received and user can customise the response to this situation.
        ESP_LOGE(TAG, "Complete data was not received.");
    } else {
        ota_finish_err = esp_https_ota_finish(https_ota_handle);
        if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
            ESP_LOGI(TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_restart();
        } else {
            if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
                ESP_LOGE(TAG, "Image validation failed, image is corrupted");
            }
            ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed 0x%x", ota_finish_err);
            esp_restart();
            vTaskDelete(NULL);
        }
    }

ota_end:
    esp_https_ota_abort(https_ota_handle);
    ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed");
    vTaskDelete(NULL);
}

void app_main(void)
{
    // sys_delay_ms (10000);
    gsm_gpio_config ();

    tinyusb_config_t tusb_cfg = { 0 }; // the configuration uses default values
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t amc_cfg = { 0 }; // the configuration uses default values
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&amc_cfg));  
    
    ESP_LOGI(TAG, "USB initialization DONE");

#if CONFIG_LWIP_PPP_PAP_SUPPORT
    esp_netif_auth_type_t auth_type = NETIF_PPP_AUTHTYPE_PAP;
#elif CONFIG_LWIP_PPP_CHAP_SUPPORT
    esp_netif_auth_type_t auth_type = NETIF_PPP_AUTHTYPE_CHAP;
#elif !defined(CONFIG_EXAMPLE_MODEM_PPP_AUTH_NONE)
#error "Unsupported AUTH Negotiation"
#endif
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &on_ip_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(NETIF_PPP_STATUS, ESP_EVENT_ANY_ID, &on_ppp_changed, NULL));

    event_group = xEventGroupCreate();
    GSM_Sem = xSemaphoreCreateMutex();
   
    /* create dte object */
    esp_modem_dte_config_t config = ESP_MODEM_DTE_DEFAULT_CONFIG();
    /* setup UART specific configuration based on kconfig options */

    config.tx_io_num = CONFIG_EXAMPLE_MODEM_UART_TX_PIN;
    config.rx_io_num = CONFIG_EXAMPLE_MODEM_UART_RX_PIN;
    config.rts_io_num = 0;
    config.cts_io_num = 0;
    config.rx_buffer_size = CONFIG_EXAMPLE_MODEM_UART_RX_BUFFER_SIZE;
    config.tx_buffer_size = CONFIG_EXAMPLE_MODEM_UART_TX_BUFFER_SIZE;
    config.event_queue_size = CONFIG_EXAMPLE_MODEM_UART_EVENT_QUEUE_SIZE;
    config.event_task_stack_size = CONFIG_EXAMPLE_MODEM_UART_EVENT_TASK_STACK_SIZE;
    config.event_task_priority = CONFIG_EXAMPLE_MODEM_UART_EVENT_TASK_PRIORITY;
    config.dte_buffer_size = CONFIG_EXAMPLE_MODEM_UART_RX_BUFFER_SIZE / 2;
    
    dte = esp_modem_dte_init(&config);
    if (dte == NULL)
    {
        ESP_LOGI (TAG, "DTE INIT FAIL");
    }
    /* Register event handler */
    ESP_ERROR_CHECK(esp_modem_set_event_handler(dte, modem_event_handler, ESP_EVENT_ANY_ID, NULL));

    // Init netif object
    // esp_netif_config_t cfg = ESP_NETIF_DEFAULT_PPP();
    // esp_netif_t *esp_netif = esp_netif_new(&cfg);
    // assert(esp_netif);
    // void *modem_netif_adapter;
    modem_dce_t *dce;
    /* Config MQTT */
    esp_mqtt_client_config_t mqtt_config = {
        .uri = BROKER_URL,
        .username = "mqtt",
        .password = "Thucanh!@", 
        .event_handle = mqtt_event_handler,
    };
//     {
//         dce = NULL;
//             /* create dce object */
// #if CONFIG_EXAMPLE_MODEM_DEVICE_SIM800
//         //dce = sim800_init(dte);
// #elif CONFIG_EXAMPLE_MODEM_DEVICE_BG96

// #elif CONFIG_EXAMPLE_MODEM_DEVICE_SIM7600
//         dce = sim7600_init(dte);
// #else
// #error "Unsupported DCE"
// #endif
        
//         dce = ec600s_init (dte);
//         // assert(dce != NULL);
//         if(dce == NULL)
//         {
//             ESP_LOGE (TAG,"DCE NULL");
//             protocol_using = WIFI_PROTOCOL;
//             // continue;
//         }
//         xSemaphoreTake(GSM_Sem, portMAX_DELAY);
//         ESP_LOGI (TAG, "SEM TOOK");
//         app_wifi_connect (CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
//         while (1)
//         {
//             EventBits_t gsm_bits = xEventGroupWaitBits(event_group, CONNECT_BIT , pdTRUE, pdTRUE, 1000 / portTICK_RATE_MS);
//             EventBits_t wifi_bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT , pdFALSE, pdFALSE, 1000 / portTICK_RATE_MS);
//             if (gsm_bits & CONNECT_BIT)
//             {
//                 protocol_using = GSM_4G_PROTOCOL;
//                 break;
//             }
//             else if (wifi_bits & WIFI_CONNECTED_BIT)
//             {
//                 ESP_LOGI (TAG, "WIFI CONNECTED");
//                 protocol_using = WIFI_PROTOCOL;
//                 break;
//             }
//             else if (wifi_bits & WIFI_FAIL_BIT)
//             {
//                 ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
//                         CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
//                 protocol_using = GSM_4G_PROTOCOL;
//                 break;
//             }
//             else
//             {
//                 ESP_LOGE(TAG, "UNEXPECTED EVENT");
//                 protocol_using = GSM_4G_PROTOCOL;
//                 break;
//             }
//         }            
//     }
    EventBits_t ubits;
    while (1) {
        //khoi tao ngoai roi check o day=> lang phí
        /*
        if (network_connected) 
        {
            // check_update_bit
            ubits = xEventGroupWaitBits (event_group, WAIT_BIT, pdTRUE, pdTRUE, 100);
            if (ubits & WAIT_BIT)
            {
                xTaskCreate(&advanced_ota_example_task, "advanced_ota_example_task", 1024 * 8, NULL, 5, NULL);
            }
        }
        */
        switch (protocol_using)
        {
            case GSM_4G_PROTOCOL:
            ESP_LOGI (TAG, "BEGIN 4G");
            dce = NULL;
            /* create dce object */
#if CONFIG_EXAMPLE_MODEM_DEVICE_SIM800
        //dce = sim800_init(dte);
#elif CONFIG_EXAMPLE_MODEM_DEVICE_BG96

#elif CONFIG_EXAMPLE_MODEM_DEVICE_SIM7600
        dce = sim7600_init(dte);
#else
#error "Unsupported DCE"
#endif
        
            if(dce == NULL)
            dce = ec600s_init (dte);
            // assert(dce != NULL);
            if(dce == NULL)
            {
                protocol_using = WIFI_PROTOCOL;
                continue;
            }
            xSemaphoreTake(GSM_Sem, portMAX_DELAY);

            /* Wait for IP address */
            xEventGroupWaitBits(event_group, CONNECT_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
            // network_connected = true;
            
            esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_config);
            esp_mqtt_client_start(mqtt_client);
            while (1)
            {
                EventBits_t uxBits = xEventGroupWaitBits(event_group, MQTT_DIS_CONNECT_BIT, pdTRUE, pdTRUE, 10);
                if ( (uxBits & MQTT_DIS_CONNECT_BIT) == MQTT_DIS_CONNECT_BIT)
                {
                    ESP_LOGI (TAG, "mqtt disconnected bit rev");
                    // network_connected = false;
                    protocol_using = WIFI_PROTOCOL;
                    break;
                }
                xEventGroupWaitBits(event_group, GOT_DATA_BIT, pdTRUE, pdTRUE, 1000 / portTICK_RATE_MS);
                ubits = xEventGroupWaitBits (event_group, WAIT_BIT, pdTRUE, pdTRUE, 100);
                if (ubits & WAIT_BIT)
                {
                    xTaskCreate(&advanced_ota_example_task, "advanced_ota_example_task", 1024 * 8, NULL, 5, NULL);
                }
            }
            esp_mqtt_client_destroy(mqtt_client);
            ESP_LOGI(TAG, "destroy mqtt");
            /* Exit PPP mode */
            ESP_ERROR_CHECK(esp_modem_stop_ppp(dte));
            ESP_LOGI(TAG, "ppp stop");
            xEventGroupWaitBits(event_group, STOP_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
            ESP_LOGI(TAG, "got bit");
#if CONFIG_EXAMPLE_SEND_MSG
            const char *message = "Welcome to ESP32!";
            ESP_ERROR_CHECK(example_send_message_text(dce, CONFIG_EXAMPLE_SEND_MSG_PEER_PHONE_NUMBER, message));
            ESP_LOGI(TAG, "Send send message [%s] ok", message);
#endif
            /* Power down module */
            // ESP_ERROR_CHECK(dce->power_down(dce));
            ESP_LOGI(TAG, "Power down");
            ESP_ERROR_CHECK(dce->deinit(dce));

            ESP_LOGI(TAG, "Restart after 60 seconds");
            vTaskDelay(pdMS_TO_TICKS(10000));
            
            if (esp_modem_netif_clear_default_handlers(modem_netif_adapter) == ESP_OK)
            {
                ESP_LOGI ("UNREGISTER", "CLEAR TO REINIT");
            }
            esp_modem_netif_teardown(modem_netif_adapter);
            esp_netif_destroy(esp_netif);
            ESP_LOGI ("UNREGISTER", "CLEAR TO REINIT");
            break;

            case WIFI_PROTOCOL:
                ESP_LOGI (TAG, "BEGIN WIFI");
                app_wifi_connect (CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
                ESP_LOGI (TAG, "WIFI CONNECT");
                do_ping_cmd(); //pinging to addr 
                EventBits_t uxBitsPing = xEventGroupWaitBits(s_wifi_event_group, WIFI_PING_TIMEOUT | WIFI_PING_SUCESS, pdTRUE, pdFALSE, portMAX_DELAY);
                if (uxBitsPing & WIFI_PING_TIMEOUT)
                {
                    ESP_LOGI (TAG, "PING TIMEOUT");
                    protocol_using = GSM_4G_PROTOCOL;
                    break;
                }
                else if (uxBitsPing & WIFI_PING_SUCESS)
                {
                    ESP_LOGI (TAG, "PING OK");
                    // network_connected = true;
                }
                else
                {
                    ESP_LOGI (TAG, "PING UNEXPECTED EVENT");
                }
                mqtt_client = esp_mqtt_client_init(&mqtt_config);
                esp_mqtt_client_start(mqtt_client);
                while (1)
                {
                    EventBits_t uxBits = xEventGroupWaitBits(event_group, MQTT_DIS_CONNECT_BIT, pdTRUE, pdTRUE, 10);
                    if ( (uxBits & MQTT_DIS_CONNECT_BIT) == MQTT_DIS_CONNECT_BIT)
                    {
                        ESP_LOGI (TAG, "mqtt disconnected bitrev");
                        // network_connected = false;
                        protocol_using = GSM_4G_PROTOCOL;
                        app_time();
                        vTaskDelay(500 / portTICK_PERIOD_MS);
                        //break;
                    }
                    xEventGroupWaitBits(event_group, GOT_DATA_BIT, pdTRUE, pdTRUE, 1000 / portTICK_RATE_MS);
                    ubits = xEventGroupWaitBits (event_group, WAIT_BIT, pdTRUE, pdTRUE, 100);
                    if (ubits & WAIT_BIT)
                    {
                        xTaskCreate(&advanced_ota_example_task, "advanced_ota_example_task", 1024 * 8, NULL, 5, NULL);
                    }
                }
                esp_mqtt_client_destroy(mqtt_client);
                ESP_LOGI(TAG, "destroy mqtt");
                esp_wifi_stop();
            break;
            default:
            break;
        }
    }
    ESP_ERROR_CHECK(dte->deinit(dte));
}
