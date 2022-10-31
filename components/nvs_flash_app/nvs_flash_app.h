#ifndef NSV_FLASH_APP
#define NSV_FLASH_APP

#include "nvs_flash.h"
#include "nvs.h"
#include "app_mqtt.h"

#define INTERNAL_FLASH_NVS_KEY_TOPIC_HEADER "topic"
#define INTERNAL_FLASH_NVS_KEY_MQTT_ADDR "mqttAddr"
#define INTERNAL_FLASH_NVS_KEY_MQTT_USERNAME "mqttUser"
#define INTERNAL_FLASH_NVS_KEY_MQTT_PASSWORD "mqttPass"
#define INTERNAL_FLASH_NVS_KEY_CHARG_INTERVAL "charging"
#define INTERNAL_FLASH_NVS_KEY_UNCHARG_INTERVAL "uncharge"
#define INTERNAL_FLASH_NVS_KEY_PHONE_1 "Phone1"
#define INTERNAL_FLASH_NVS_KEY_PHONE_2 "Phone2"
#define INTERNAL_FLASH_NVS_KEY_PHONE_3 "Phone3"
#define INTERNAL_FLASH_NVS_KEY_BUZZ_EN "buzzerEn"
#define INTERNAL_FLASH_NVS_KEY_SYNC_ALARM "syncAlarm"
#define INTERNAL_FLASH_NVS_KEY_NETWORK_ADDR "networkAddr"
#define INTERNAL_FLASH_NVS_KEY_SMOKE_WAKE "smokeWake"
#define INTERNAL_FLASH_NVS_KEY_SMOKE_HEARTBEAT "smokeHeartbeat"
#define INTERNAL_FLASH_NVS_KEY_SMOKE_THRESH "smokeThresHo"
#define INTERNAL_FLASH_NVS_KEY_TEMP_WAKE "tempWakeup"
#define INTERNAL_FLASH_NVS_KEY_TEMP_HEARTBEAT "tempHeartbeat"
#define INTERNAL_FLASH_NVS_KEY_TEMP_THRESH "tempThreshH"
#define INTERNAL_FLASH_NVS_KEY_DNS_NAME "httpDns"
#define INTERNAL_FLASH_NVS_KEY_DNS_USER "httpUser"
#define INTERNAL_FLASH_NVS_KEY_DNS_PASS "httpPass"
#define INTERNAL_FLASH_NVS_KEY_DNS_PORT "httpPort"
#define INTERNAL_FLASH_NVS_KEY_WIFI_NAME "wifiname"
#define INTERNAL_FLASH_NVS_KEY_WIFI_PASS "wifipass"
#define INTERNAL_FLASH_NVS_KEY_WIFI_DIS "wifiDis"
#define INTERNAL_FLASH_NVS_KEY_PING_MAIN_SERVER "pingMain"
#define INTERNAL_FLASH_NVS_KEY_PING_BACKUP_SERVER "pingBackup"
#define INTERNAL_FLASH_NVS_KEY_INPUT_ACTIVE_LEVEL "inputAct"

#define MQTT_SEVER_KEY_TOPIC_HEADER "topicHeader"
#define MQTT_SEVER_KEY_MQTT_ADDR "mqttAddress"
#define MQTT_SEVER_KEY_MQTT_USERNAME "mqttUserName"
#define MQTT_SEVER_KEY_MQTT_PASSWORD "mqttPassword"
#define MQTT_SEVER_KEY_CHARG_INTERVAL "chargingInterval"
#define MQTT_SEVER_KEY_UNCHARG_INTERVAL "unchargeInterval"
#define MQTT_SEVER_KEY_PHONE_1 "userPhoneNumber1"
#define MQTT_SEVER_KEY_PHONE_2 "userPhoneNumber2"
#define MQTT_SEVER_KEY_PHONE_3 "userPhoneNumber3"
#define MQTT_SEVER_KEY_BUZZ_EN "buzzerEnable"
#define MQTT_SEVER_KEY_SYNC_ALARM "syncAlarm"
#define MQTT_SEVER_KEY_NETWORK_ADDR "networkAddress"
#define MQTT_SEVER_KEY_SMOKE_WAKE "smokeSensorWakeupInterval"
#define MQTT_SEVER_KEY_SMOKE_HEARTBEAT "smokeSensorHeartbeatInterval"
#define MQTT_SEVER_KEY_SMOKE_THRESH "smokeSensorThresHole"
#define MQTT_SEVER_KEY_TEMP_WAKE "tempSensorWakeupInterval"
#define MQTT_SEVER_KEY_TEMP_HEARTBEAT "tempSensorHeartbeatInterval"
#define MQTT_SEVER_KEY_TEMP_THRESH "tempThresHold"
#define MQTT_SEVER_KEY_DNS_NAME "httpDnsName"
#define MQTT_SEVER_KEY_DNS_USER "httpUsername"
#define MQTT_SEVER_KEY_DNS_PASS "httpDnsPass"
#define MQTT_SEVER_KEY_DNS_PORT "httpDnsPort"
#define MQTT_SEVER_KEY_WIFI_NAME "wifiname"
#define MQTT_SEVER_KEY_WIFI_PASS "wifipass"
#define MQTT_SEVER_KEY_WIFI_DIS "wifiDisable"
#define MQTT_SEVER_KEY_PING_MAIN_SERVER "pingMainServer"
#define MQTT_SEVER_KEY_PING_BACKUP_SERVER "pingBackupServer"
#define MQTT_SEVER_KEY_INPUT_ACTIVE_LEVEL "inputActiveLevel"


typedef struct 
{
    uint8_t device_mac [6];
    uint8_t device_type;
    uint16_t unicast_add;
} __attribute__((packed)) node_sensor_data_t;

typedef struct __attribute ((packed))
{
   uint8_t device_mac[6];
   uint8_t device_type;
   bool pair_success;
} beacon_pair_info_t;

typedef enum
{
    STRING_TYPE,
    INT_TYPE,
    BOOL_TYPE
} type_of_mqtt_data_t;

typedef enum 
{
    TOPIC_HDR,
    MQTT_ADDR,
    MQTT_USER,
    MQTT_PW,
    CHARG_INTERVAL,
    UNCHARG_INTERVAL,
    PHONE_NUM_1,
    PHONE_NUM_2,
    PHONE_NUM_3,
    BUZZ_EN,
    SYNC_ALARM,
    NETWORK_ADDR,
    SMOKE_WAKE,
    SMOKE_HEARTBEAT,
    SMOKE_THRESH,
    TEMPER_WAKE,
    TEMPER_HEARTBEAT,
    TEMPER_THRESH,
    DNS_NAME,
    DNS_USER,
    DNS_PASS,
    DNS_PORT,
    WIFI_NAME,
    WIFI_PASS,
    WIFI_DIS,
    PING_MAIN_SERVER,
    PING_BACKUP_SERVER,
    INPUT_LEVEL,
    MAX_CONFIG_HANDLE
} mqtt_config_list;


void build_string_from_MAC (uint8_t* device_mac, char * out_string);
uint16_t get_and_store_new_unicast_addr_now (void);
uint16_t find_and_write_into_mac_key_space (node_sensor_data_t* data_write, size_t* byte_write, char* mac);
void init_nvs_flash(void );
int32_t write_data_to_flash(void *data_write, size_t byte_write, const char* key);
esp_err_t read_data_from_flash (void *data_read, uint16_t* byte_read, const char* key);
type_of_mqtt_data_t get_type_of_data (mqtt_config_list mqtt_config_element);
void make_key_from_mqtt_list_type (char* str_out, mqtt_config_list mqtt_config_element);
void make_key_to_store_type_in_nvs (char* str_out, mqtt_config_list mqtt_config_element);
int32_t internal_flash_nvs_get_u16(char *key, uint16_t *value);
int32_t internal_flash_nvs_write_u16(char *key, uint16_t value);
int32_t internal_flash_nvs_read_string(char *key, char *buffer, uint32_t max_size);
int32_t internal_flash_nvs_write_string(char *key, char *value);
int32_t internal_flash_nvs_write_u32(char *key, uint32_t value);
int32_t internal_flash_nvs_get_u32(char *key, uint32_t *value);
/**
 * @brief       Write uint8_t value to nvs flash
 * @param[in]   key NVS key
 * @param[in]   value Value write to NVS
 * @retval      esp_err_t
 */
int32_t internal_flash_nvs_write_u8(char *key, uint8_t value);

/**
 * @brief       Read uint8_t value from nvs flash
 * @param[in]   key NVS key
 * @param[in]   value Value read from NVS
 * @retval      esp_err_t
 */
int32_t internal_flash_nvs_get_u8(char *key, uint8_t *value);

uint32_t read_config_data_from_flash (info_config_t* config, type_of_mqtt_data_t data_type, mqtt_config_list mqtt_config_list);

#endif