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
    uint32_t updateTime;
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
    char hardwareVersion[20];
    int updateTime;
    char ExpFwVersion[10];
    char ExpHwVersion[10];
}__attribute__((packed)) info_device_t;

typedef struct 
{
    char mac[6];
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
}__attribute__((packed)) info_config_from_server_t;

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

// msg format get from nrt module

typedef struct __attribute((packed))
{
  uint32_t alarm_value;
  uint8_t sensor_count;
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

typedef union __attribute((packed))
{
    struct __attribute((packed))
    {
        uint8_t tid : 4;
        uint8_t version : 4;    // limited 16 version
    } info;
    uint8_t value;
} app_beacon_tid_t;

typedef union __attribute((packed))
{
    struct Properties_t
    {
        uint16_t deviceType : 6;        
        uint16_t alarmState : 1;
        uint16_t isNewMsg : 1;
        uint16_t isPairMsg : 1;
        //uint16_t fwVersion : 5;
        uint16_t comboSensor : 1;
        uint16_t reserve : 10;
    } Name;
    uint16_t Value;
}node_proprety_t;

typedef struct
{
    uint8_t Data[14];
    uint8_t Len;
    uint8_t IsCustom;
} node_custom_data_t;

typedef struct __attribute((packed))
{
  uint8_t device_mac[6];
  //uint8_t device_type;// Maybe Bitfield
  app_beacon_tid_t beacon_tid;
  uint8_t battery;
  uint8_t is_data_valid;  // Maybe Bitfield 
  uint8_t fw_verison;
  //uint8_t msg_type;
  uint8_t teperature_value;
  uint8_t smoke_value;
  uint32_t timestamp; /*<Current tick when receive this value>*/
  node_proprety_t propreties; /*Node's Propreties*/
  node_custom_data_t custom_data;
} app_beacon_data_t;

typedef struct
{
    uint8_t appkey[16]; // KEY_LENGHT_16
    uint8_t netkey[16];
    uint32_t iv_index;
    uint32_t sequence_number;
} ble_info_t;


void make_mqtt_topic_header(header_type_t header_type ,char *topic_hr, char *IMEI, char* str_out);

void make_fire_status_payload (fire_status_t info, char * str_out);
void make_firealarm_payload (fire_safe_t info_fire_alarm, char *str_out);
void make_sensor_info_payload (sensor_info_t sensor, char *str_out);
void make_device_info_payload (info_device_t device, char * str_out);
void make_config_info_payload (info_config_t config, char * str_out);



#endif