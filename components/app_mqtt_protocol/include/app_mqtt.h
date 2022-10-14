#ifndef APP_MQTT_T
#define APP_MQTT_T

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
//#include "uthash.h"

typedef union
{
    struct 
    {
        uint8_t gsm_internet : 1;
        uint8_t eth_internet : 1;
        uint8_t wifi_internet : 1;
        uint8_t reserve : 5;
    } __attribute__((packed)) name;
    uint8_t value;
} __attribute__((packed)) interface_internet_t;


typedef struct 
{
    /* data */
    int fire_status;
    int csq;
    int sensor_cnt;
    int battery;
    int updateTime;
    char networkInterface[12];
    int temper;
    int fire_zone;
    interface_internet_t networkStatus;
    bool inTestmode;
} __attribute__((packed)) fire_status_t;

typedef struct 
{
    int fire_status;
    int csq;
    int sensor_cnt;
    int battery;
    int updateTime;
    bool inTestmode;
}__attribute__((packed)) fire_safe_t;

typedef struct 
{
    char imei[20];
    char simIMEI[20];
    char firmware[20];
    char loginReson[10];
    int hardwareVersion;
    int updateTime;
    char ExpFwVersion[10];
    char ExpHwVersion[10];
}__attribute__((packed)) info_device_t;

typedef struct 
{
    char mac[20];
    char firmware[20];
    int status;
    int battery;
    int temperature;
    int smoke;
    int updateTime;
}__attribute__((packed)) sensor_info_t;


typedef struct 
{
    char topic_hr [20];
    char mqtt_add [20];
    char mqtt_user [20];
    char mqtt_pass [20];
    int charg_interval;
    int uncharg_interval;
    char userPhoneNumber1 [10];
    char userPhoneNumber2 [10];
    char userPhoneNumber3 [10];
    bool buzzerEnable;
    bool syncAlarm;
    char networkAddress[20];
    int smokeSensorWakeupInterval;
    int smokeSensorHeartbeatInterval;
    int smokeSensorThresHole;
    int tempSensorWakeupInterval;
    int tempSensorHeartbeatInterval;
    int tempThresHold;
    char httpDnsName [15];
    char httpUsername [15];
    char httpDnsPass [15];
    int httpDnsPort;
    char wifiname [15];
    char wifipass [15];
    int wifiDisable;
    int reset;
    char pingMainServer[15];
    char pingBackupServer [15];
    int inputActiveLevel;
    int zoneMinMv;
    int zoneMaxMv;
    int zoneDelay;
}__attribute__((packed)) info_config_t;

typedef enum 
{
    HEART_BEAT_HEADER,
    FIRE_ALARM_HEADER,
    SENSOR_HEADER,
    INFO_HEADER,
    CONFIG_HEADER,
    OTA_HEADER
} header_type_t;

typedef struct __attribute((packed))
{
    uint8_t netkey[16];
    uint8_t appkey[16];
} app_provision_key_t;

typedef struct
{
    app_provision_key_t provision;
    uint32_t iv_index;
    uint32_t sequence_number;
}__attribute((packed)) ble_config_t;


typedef struct 
{
    header_type_t header;
    char *payload;
} app_mqtt_msg_t;


typedef struct __attribute((packed))
{
  uint32_t alarm_value;
  uint16_t sensor_count;
  uint8_t gateway_mac[6];
  app_provision_key_t mesh_key;
  uint8_t in_pair_mode;
}app_beacon_ping_msg_t;
/*
typedef struct __attribute((packed))
{
  uint8_t device_mac[6];
  uint8_t device_type;// Maybe Bitfield
  app_beacon_tid_t beacon_tid;
  uint8_t battery;
}app_beacon_msg_t;
*/
void make_mqtt_topic_header(header_type_t header_type ,char *topic_hr, char *IMEI, char* str_out);

#endif