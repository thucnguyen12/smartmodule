#include "app_mqtt.h"
#include <string.h>
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "mqtt_protocol";

void make_mqtt_topic_header(header_type_t header_type ,char *topic_hr, char *IMEI, char* str_out)
{
    // switch case
    
    switch (header_type)
    {
    case HEART_BEAT_HEADER:
        sprintf (str_out, "%s/d2g/heartbeat/%s", topic_hr, IMEI);
        ESP_LOGI(TAG, "creat heart beat header");
        break;
    case FIRE_ALARM_HEADER:
        sprintf (str_out, "%s/d2g/fire/%s", topic_hr, IMEI);
        ESP_LOGI(TAG, "creat fire header");
        break;
    case SENSOR_HEADER:
        sprintf (str_out, "%s/d2g/sensor/%s", topic_hr, IMEI);
        ESP_LOGI(TAG, "creat sensor header");
        break;
    case INFO_HEADER:
        sprintf (str_out, "%s/d2g/info/%s", topic_hr, IMEI);
        ESP_LOGI(TAG, "creat info header");
        break;
    case CONFIG_HEADER:
        sprintf (str_out, "%s/d2g/config/%s", topic_hr, IMEI);
        ESP_LOGI(TAG, "creat config header");
        break;
    case OTA_HEADER:
        sprintf (str_out, "%s/d2g/ota/%s", topic_hr, IMEI);
        ESP_LOGI(TAG, "creat ota header");
        break;
    default:
        break;
    }
}

void make_fire_status_payload (fire_status_t info, char * str_out)
{
    uint32_t index = 0;
    index = sprintf (str_out + index, "{\r\n\"fireStatus\":%u\r\n", info.fire_status);
    index = sprintf (str_out + index, "\"csq\":%u\r\n", info.csq);
    index = sprintf (str_out + index, "\"connectedSenSor\":%u\r\n", info.sensor_cnt);
    index = sprintf (str_out + index, "\"battery\":%u\r\n", info.battery);
    index = sprintf (str_out + index, "\"updateTime\":%u\r\n", info.updateTime);
    index = sprintf (str_out + index, "\"InTestMode\":%s\r\n", info.inTestmode ? "1":"0");
    index = sprintf (str_out + index, "\"temperature\":%s\r\n", info.inTestmode ? "1":"0");
    index = sprintf (str_out + index, "\"networkInterface\":%s\r\n", info.networkInterface);
    index = sprintf (str_out + index, "\"fireZone\":%x\r\n}",info.fire_zone);
}

void make_firealarm_payload (fire_safe_t info_fire_alarm, char *str_out)
{
    uint32_t index = 0;
    index = sprintf (str_out + index, "{\r\n\"Fire_status\":%u\r\n", info_fire_alarm.fire_status);
    index = sprintf (str_out + index, "\"csq\":%u\r\n", info_fire_alarm.csq);
    index = sprintf (str_out + index, "\"connectedSensor\":%u", info_fire_alarm.sensor_cnt);
    index = sprintf (str_out + index, "\"battery\":%u\r\n", info_fire_alarm.battery);
    index = sprintf (str_out + index, "\"updateTime\":%u\r\n", info_fire_alarm.updateTime);
    index = sprintf (str_out + index, "\"InTestMode\":%s\r\n}", info_fire_alarm.inTestmode ? "1":"0");
}

void make_sensor_info_payload (sensor_info_t sensor, char *str_out)
{
    uint32_t index = 0;
    index = sprintf (str_out + index, "{\r\n\"mac\":%s\r\n", sensor.mac);
    index = sprintf (str_out + index, "\"firmware\":%s\r\n", sensor.firmware);
    index = sprintf (str_out + index, "\"status\":%u\r\n", sensor.status);
    index = sprintf (str_out + index, "\"battery\":%u\r\n", sensor.battery);
    index = sprintf (str_out + index, "\"temperature\":%u\r\n", sensor.temperature);
    index = sprintf (str_out + index, "\"smoke\":%u\r\n", sensor.smoke);
    index = sprintf (str_out + index, "\"updateTime\":%u\r\n}", sensor.updateTime);
}

void make_device_info_payload (info_device_t device, char * str_out)
{
    uint32_t index = 0;
    index = sprintf (str_out + index, "{\r\n\"imei\":%s\r\n", device.imei);
    index = sprintf (str_out + index, "\"simIMEI\":%s\r\n", device.simIMEI);
    index = sprintf (str_out + index, "\"firmware\":%s\r\n", device.firmware);
    index = sprintf (str_out + index, "\"loginReason\":%s\r\n", device.loginReson);
    index = sprintf (str_out + index, "\"hardwareVersion\":%s\r\n", device.hardwareVersion);
    index = sprintf (str_out + index, "\"updateTime\":%d\r\n", device.updateTime);
    index = sprintf (str_out + index, "\"ExpFwVersion\":%s\r\n", device.ExpFwVersion);
    index = sprintf (str_out + index, "\"ExpHwVersion\":%s\r\n}", device.ExpHwVersion);
}

void make_config_info_payload (info_config_t config, char * str_out)
{
    uint32_t index = 0;
    index = sprintf (str_out + index, "{\r\n\"topicHeader\":%s\r\n", config.topic_hr);
    index = sprintf (str_out + index, "\"mqttAddress\":%s\r\n", config.mqtt_add);
    index = sprintf (str_out + index, "\"mqttUserName\":%s\r\n", config.mqtt_user);
    index = sprintf (str_out + index, "\"mqttPassword\":%s\r\n", config.mqtt_pass);
    index = sprintf (str_out + index, "\"chargingInterval\":%d\r\n", config.charg_interval);
    index = sprintf (str_out + index, "\"unchargeInterval \":%d\r\n", config.uncharg_interval);
    index = sprintf (str_out + index, "\"userPhoneNumber1 \":%s\r\n", config.userPhoneNumber1);
    index = sprintf (str_out + index, "\"userPhoneNumber2 \":%s\r\n", config.userPhoneNumber2);
    index = sprintf (str_out + index, "\"userPhoneNumber3 \":%s\r\n", config.userPhoneNumber3);
    index = sprintf (str_out + index, "\"buzzerEnable \":%s\r\n", config.buzzerEnable ? "true" : "false");
    index = sprintf (str_out + index, "\"syncAlarm\":%s\r\n", config.syncAlarm ? "true" : "false");
    index = sprintf (str_out + index, "\"networkAddress\":%s\r\n", config.networkAddress);
    index = sprintf (str_out + index, "\"smokeSensorWakeupInterval\":%d\r\n", config.smokeSensorWakeupInterval);
    index = sprintf (str_out + index, "\"smokeSensorHeartbeatInterval\":%d\r\n", config.smokeSensorHeartbeatInterval);
    index = sprintf (str_out + index, "\"smokeSensorThresHole\":%d\r\n", config.smokeSensorThresHole);
    index = sprintf (str_out + index, "\"tempSensorWakeupInterval\":%d\r\n", config.tempSensorWakeupInterval);
    index = sprintf (str_out + index, "\"tempThresHold\":%d\r\n", config.tempThresHold);
    index = sprintf (str_out + index, "\"httpDnsName\":%s\r\n", config.httpDnsName);
    index = sprintf (str_out + index, "\"httpUsername\":%s\r\n", config.httpUsername);
    index = sprintf (str_out + index, "\"httpDnsPass\":%s\r\n", config.httpDnsPass);
    index = sprintf (str_out + index, "\"httpDnsPort\":%d\r\n", config.httpDnsPort);
    index = sprintf (str_out + index, "\"wifiname\":%s\r\n", config.wifiname);
    index = sprintf (str_out + index, "\"wifipass\":%s\r\n", config.wifipass);
    index = sprintf (str_out + index, "\"wifiDisable\":%d\r\n", config.wifiDisable);
    //index = sprintf (str_out + index, "\"reset\":%d\r\n", config.reset);
    index = sprintf (str_out + index, "\"pingMainServer\":%s\r\n", config.pingMainServer);
    index = sprintf (str_out + index, "\"pingBackupServer\":%s\r\n", config.pingBackupServer);
    index = sprintf (str_out + index, "\"inputActiveLevel\":%d\r\n", config.inputActiveLevel);
    index = sprintf (str_out + index, "\"zoneMinMv\":%d\r\n", config.zoneMinMv);
    index = sprintf (str_out + index, "\"zoneMaxMv\":%d\r\n", config.zoneMaxMv);
    index = sprintf (str_out + index, "\"zoneDelay\":%d\r\n}", config.zoneDelay);
}


// typedef struct
// {
//    //bool available; check if inf is saved in flash
//    uint8_t app_key[KEY_LENGHT]; // KEY_LENGHT_16
//    uint8_t net_key[KEY_LENGHT];
//    dsm_local_unicast_address_t unicast_address;
//    //uint32_t sum32;
// }mesh_network_info_t;



