#ifndef NSV_FLASH_APP
#define NSV_FLASH_APP

#include "nvs_flash.h"
#include "nvs.h"

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

const char* key_table [] = {
    "topicHeader"
    "mqttAddress",
    "mqttUserName",
    "mqttPassword",
    "chargingInterval",
    "unchargeInterval",
    "userPhoneNumber1",
    "userPhoneNumber2",
    "userPhoneNumber3",
    "buzzerEnable",
    "syncAlarm",
    "networkAddress",
    "smokeSensorWakeupInterval",
    "smokeSensorHeartbeatInterval",
    "smokeSensorThresHole",
    "tempSensorWakeupInterval",
    "tempSensorHeartbeatInterval",
    "tempThreshHold",
    "httpDnsName",
    "httpUsername",
    "httpDnsPass", 
    "httpDnsPort",
    "wifiname",
    "wifipass",
    "wifiDisable",
    "pingMainServer",
    "pingBackupServer",
    "inputActiveLevel"
};


void build_string_from_MAC (uint8_t* device_mac, char * out_string);
uint16_t get_and_store_new_unicast_addr_now (void);
uint16_t find_and_write_into_mac_key_space (node_sensor_data_t* data_write, size_t* byte_write, char* mac);
void init_nvs_flash(void );
int32_t write_data_to_flash(void *data_write, size_t byte_write, const char* key);
esp_err_t read_data_from_flash (void *data_read, uint16_t* byte_read, const char* key);
type_of_mqtt_data_t get_type_of_data (char * key);
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

void read_config_data_from_flash (info_config_t* config, type_of_mqtt_data_t data_type, mqtt_config_list mqtt_config_list);

#endif