#include "nvs_flash_app.h"
#include "esp_log.h"
#include "esp_err.h"
#include <string.h>
#include <stdbool.h>
static const char* TAG = "nvs_flash_app";
static int32_t m_err_code = ESP_OK;



type_of_mqtt_data_t get_type_of_data (char * key)
{
    type_of_mqtt_data_t type;
    
    if (strstr (key, "buzzerEnable") || strstr (key, syncAlarm))
    {
        type = BOOL_TYPE;
    }
    else if (strstr (key, "topicHeader")
            || strstr (key, "mqttAddress") 
            || strstr (key, "mqttUserName")
            || strstr (key, "mqttPassword")
            || strstr (key, "userPhoneNumber1")
            || strstr (key, "userPhoneNumber2")
            || strstr (key, "userPhoneNumber3")
            || strstr (key, "httpDnsName")
            || strstr (key, "httpDnsPass")
            || strstr (key, "wifiname")
            || strstr (key, "wifipass")
            || strstr (key, "pingMainServer")
            || strstr (key, "pingMainServer")
            )
    {
        type = STRING_TYPE;
    }
    else
    {
        type = INT_TYPE;
    }
    return type;
}


void write_data_into_key_space (char * str_config)
{
    for (mqtt_config_list i = 0; i < MAX_CONFIG_HANDLE; i++)
    {
    
    }

}

// typedef struct 
// {
//     uint8_t device_mac [6];
//     uint8_t device_type;
//     uint16_t unicast_add;
// } __attribute__((packed)) node_sensor_data_t;

// typedef struct __attribute ((packed))
// {
//    uint8_t device_mac[6];
//    uint8_t device_type;
//    bool pair_success;
// } beacon_pair_info_t;

void build_string_from_MAC (uint8_t* device_mac, char * out_string)
{
    sprintf(out_string, "%02x:%02x:%02x:%02x:%02x:%02x", device_mac[0],
                                                        device_mac[1],
                                                        device_mac[2],
                                                        device_mac[3],
                                                        device_mac[4],
                                                        device_mac[5]);
}

#define UNICAST_ADD_STORE_KEY "unicast_addr_key"
#define UNICAST_ADDR_BASE 0X1000



uint16_t get_and_store_new_unicast_addr_now (void)
{
    uint16_t unicast_addr = UNICAST_ADDR_BASE;
    if (ESP_OK != internal_flash_nvs_get_u16(UNICAST_ADD_STORE_KEY, &unicast_addr))
    {
        internal_flash_nvs_write_u16 (UNICAST_ADD_STORE_KEY, UNICAST_ADDR_BASE);
    }
    else
    {
        unicast_addr += 4;//dinh nghia lai ve khoang cach danh dia chi
        internal_flash_nvs_write_u16 (UNICAST_ADD_STORE_KEY, unicast_addr);
    }
    return unicast_addr;
}

uint16_t find_and_write_into_mac_key_space (node_sensor_data_t* data_write, size_t *byte_write, char* mac)
{
    nvs_handle_t my_handle;
    esp_err_t ret = ESP_OK;
    node_sensor_data_t data_sensor;
    uint16_t unicast_addr = 0;
    //OPEN FLASH
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    ret |= err; 
    if (err == ESP_OK) 
    {
        err = nvs_get_blob (my_handle, mac, &data_sensor, byte_write);
        switch (err) {
        case ESP_OK:
            unicast_addr = data_sensor.unicast_add;
            ESP_LOGI(TAG,"Done\n");
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI(TAG,"The value is not initialized yet! Creat new unicast addr\n");
            unicast_addr = get_and_store_new_unicast_addr_now();
            data_write->unicast_add = unicast_addr;
            nvs_set_blob (my_handle, mac, data_write, sizeof (node_sensor_data_t));
            break;
        // case ESP_ERR_NVS_KEYS_NOT_INITIALIZED:
        //     ESP_LOGI("The value is not initialized yet!\n");
        //     break;
        default :
            ESP_LOGI(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
        }
        nvs_close(my_handle);
    }
    else
    {
        ESP_LOGI(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    return unicast_addr;
}

//******************************************* NVS PLACE *****************************///
void init_nvs_flash(void )
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

//  write data in flash
int32_t write_data_to_flash(void *data_write, size_t byte_write, const char* key)
{
    nvs_handle_t my_handle;
    esp_err_t ret = ESP_OK;
    //OPEN FLASH
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    ret |= err;

    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        // ghi lại gia tri mac dinh và them version control
    } else {
        ESP_LOGI(TAG, "OPEN Done, Writing data to key %s\n", key);
        err = nvs_set_blob (my_handle, key, data_write, byte_write);
        ESP_LOGI(TAG, "%s", (err != ESP_OK) ? "Failed!" : "Done");
        ret |= err;

        ESP_LOGI(TAG, "\tCommitting updates string in NVS ... ");
        err = nvs_commit(my_handle);
        ESP_LOGI(TAG, "%s", (err != ESP_OK) ? "Failed!" : "Done");
        ret |= err; 

        nvs_close(my_handle);
    }
    return ret;
}

//read data blob from flash
esp_err_t read_data_from_flash (void *data_read, uint16_t* byte_read, const char* key)
{
    nvs_handle_t my_handle;
    esp_err_t ret = ESP_OK;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    ESP_LOGW(TAG, "READ DATA");
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        ret |= err;
    } else {
        ESP_LOGI(TAG, "OPEN OK");
        esp_err_t err = nvs_get_blob(my_handle, key, data_read, (size_t *)byte_read);
        ESP_LOGI(TAG, "get info");
        switch (err)
        {
        case ESP_OK:
            break;

        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGW(TAG, "Key %s not found", key);
            ret |= err;
            break;

        default:
            ESP_LOGE(TAG, "Error (%s) reading key %s!", esp_err_to_name(err), key);
            ret |= err;
            break;
        }
        nvs_close(my_handle);
    }
    //m_err_code = err;
    return ret;
}


int32_t internal_flash_nvs_write_string(char *key, char *value)
{
    nvs_handle nvs_handle;
    esp_err_t ret = ESP_OK;

    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    ret |= err;

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Writing key %s, value %s to NVS", key, value);

        err = nvs_set_str(nvs_handle, key, value);
        ESP_LOGI(TAG, "%s", (err != ESP_OK) ? "Failed!" : "Done");
        ret |= err;
    
        ESP_LOGI(TAG, "\tCommitting updates string in NVS ... ");
        err = nvs_commit(nvs_handle);
        ESP_LOGI(TAG, "%s", (err != ESP_OK) ? "Failed!" : "Done");
        ret |= err;

        // Close NVS
        nvs_close(nvs_handle);
    }
    else
    {
        ESP_LOGW(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    m_err_code = err;
    return ret;
}

int32_t internal_flash_nvs_read_string(char *key, char *buffer, uint32_t max_size)
{
    nvs_handle nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err == ESP_OK)
    {
        err = nvs_get_str(nvs_handle, key, buffer, (size_t*)&max_size);
        switch (err)
        {
        case ESP_OK:
            break;

        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGW(TAG, "Key %s not found", key);
            break;

        default:
            ESP_LOGE(TAG, "Error (%s) reading key %s!", esp_err_to_name(err), key);
            break;
        }
        nvs_close(nvs_handle);
    }
    m_err_code = err;
    return err;
}

int32_t internal_flash_nvs_write_u32(char *key, uint32_t value)
{
    nvs_handle nvs_handle;
    esp_err_t ret = ESP_OK;
    esp_err_t err;

    err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    ret |= err;

    if (err == ESP_OK)
    {
        uint32_t tmp = 0;
        err = nvs_get_u32(nvs_handle, key, &tmp);
        if (err != ESP_OK
            || tmp != value)
        {
            err = nvs_set_u32(nvs_handle, key, value);
            ret |= err;
            err = nvs_commit(nvs_handle);
        }
        ret |= err;
        nvs_close(nvs_handle);
    }
    
    m_err_code = err;
    if (err)
    {
        ESP_LOGW(TAG, "Error (%s) write NVS handle!", esp_err_to_name(err));
    }

    return ret;
}

int32_t internal_flash_nvs_get_u32(char *key, uint32_t *value)
{
    nvs_handle nvs_handle;
    esp_err_t ret = ESP_OK;
    esp_err_t err;

    err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    ret |= err;

    if (err == ESP_OK)
    {
        err = nvs_get_u32(nvs_handle, key, value);
        ret |= err;
        nvs_close(nvs_handle);
    }
    else
    {
        ESP_LOGW(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    m_err_code = err;
    return err;
}

int32_t internal_flash_nvs_write_u16(char *key, uint16_t value)
{
    nvs_handle nvs_handle;
    esp_err_t ret = ESP_OK;
    esp_err_t err;

    err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    ret |= err;

    if (err == ESP_OK)
    {
        uint16_t tmp = 0;
        err = nvs_get_u16(nvs_handle, key, &tmp);
        if (err != ESP_OK
            || tmp != value)
        {
            err = nvs_set_u16(nvs_handle, key, value);
            ret |= err;
            err = nvs_commit(nvs_handle);
        }
        ret |= err;
        nvs_close(nvs_handle);
    }
    
    if (err)
    {
        ESP_LOGW(TAG, "Error (%s) write NVS handle!", esp_err_to_name(err));
    }
    m_err_code = err;

    return ret;
}

int32_t internal_flash_nvs_get_u16(char *key, uint16_t *value)
{
    nvs_handle nvs_handle;
    esp_err_t ret = ESP_OK;
    esp_err_t err;

    err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    ret |= err;

    if (err == ESP_OK)
    {
        err = nvs_get_u16(nvs_handle, key, value);
        ret |= err;
        nvs_close(nvs_handle);
    }
    else
    {
        ESP_LOGW(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    m_err_code = err;
    return ret;
}

int32_t internal_flash_nvs_write_u8(char *key, uint8_t value)
{
    nvs_handle nvs_handle;
    esp_err_t ret = ESP_OK;
    esp_err_t err;

    err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    ret |= err;

    if (err == ESP_OK)
    {
        uint8_t tmp = 0;
        err = nvs_get_u8(nvs_handle, key, &tmp);
        if (err != ESP_OK
            || tmp != value)
        {
            err = nvs_set_u8(nvs_handle, key, value);
            ret |= err;
            err = nvs_commit(nvs_handle);
        }
        ret |= err;
        nvs_close(nvs_handle);
    }
    m_err_code = err;
    if (err)
    {
        ESP_LOGW(TAG, "Error (%s) write NVS handle!", esp_err_to_name(err));
    }

    return ret;
}

int32_t internal_flash_nvs_get_u8(char *key, uint8_t *value)
{
    nvs_handle nvs_handle;
    esp_err_t ret = ESP_OK;
    esp_err_t err;

    err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    ret |= err;

    if (err == ESP_OK)
    {
        err = nvs_get_u8(nvs_handle, key, value);
        ret |= err;
        nvs_close(nvs_handle);
    }
    else
    {
        ESP_LOGW(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    m_err_code = err;
    return err;
}

void read_config_data_from_flash (info_config_t* config, type_of_mqtt_data_t data_type, mqtt_config_list mqtt_config_list)
{
    switch (data_type)
    {
    case STRING_TYPE:
        /* code */
        if (mqtt_config_list == TOPIC_HDR)
            {
                internal_flash_nvs_read_string (key_table[mqtt_config_list], (config->topic_hr) ,sizeof (config->topic_hr));
                // memcpy (config_infor_now.topic_hr, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == MQTT_ADDR)
            {
                internal_flash_nvs_read_string (key_table[mqtt_config_list], (config->mqtt_add) ,sizeof (config->mqtt_add));
                // memcpy (config_infor_now.mqtt_add, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == MQTT_USER)
            {
                internal_flash_nvs_read_string (key_table[mqtt_config_list], (config->mqtt_user) ,sizeof (config->mqtt_user));
                // memcpy (config_infor_now.mqtt_pass, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == MQTT_PW)
            {
                internal_flash_nvs_read_string (key_table[mqtt_config_list], (config->mqtt_pass) ,sizeof (config->mqtt_pass));
                // memcpy (config_infor_now.mqtt_pass, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == PHONE_NUM_1)
            {
                internal_flash_nvs_read_string (key_table[mqtt_config_list], (config->userPhoneNumber1) ,sizeof (config->userPhoneNumber1));
                // memcpy (config_infor_now.userPhoneNumber1, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == PHONE_NUM_2)
            {
                internal_flash_nvs_read_string (key_table[mqtt_config_list], (config->userPhoneNumber2) ,sizeof (config->userPhoneNumber2));
                // memcpy (config_infor_now.userPhoneNumber2, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == PHONE_NUM_3)
            {
                internal_flash_nvs_read_string (key_table[mqtt_config_list], (config->userPhoneNumber3) ,sizeof (config->userPhoneNumber3));
                // memcpy (config_infor_now.userPhoneNumber3, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == DNS_NAME)
            {
                internal_flash_nvs_read_string (key_table[mqtt_config_list], (config->httpDnsName) ,sizeof (config->httpDnsName));
                // memcpy (config_infor_now.httpDnsName, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == DNS_USER)
            {
                internal_flash_nvs_read_string (key_table[mqtt_config_list], (config->httpUsername) ,sizeof (config->httpUsername));
                // memcpy (config_infor_now.httpUsername, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == DNS_PASS)
            {
                internal_flash_nvs_read_string (key_table[mqtt_config_list], (config->httpDnsPass) ,sizeof (config->httpDnsPass));
                // memcpy (config_infor_now.httpUsername, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == WIFI_NAME)
            {
                internal_flash_nvs_read_string (key_table[mqtt_config_list], (config->wifiname) ,sizeof (config->wifiname));
                // memcpy (config_infor_now.httpUsername, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == WIFI_PASS)
            {
                internal_flash_nvs_read_string (key_table[mqtt_config_list], (config->wifipass) ,sizeof (config->wifipass));
                // memcpy (config_infor_now.wifipass, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == PING_MAIN_SERVER)
            {
                internal_flash_nvs_read_string (key_table[mqtt_config_list], (config->pingMainServer) ,sizeof (config->pingMainServer));
                // memcpy (config_infor_now.pingMainServer, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == PING_BACKUP_SERVER)
            {
                internal_flash_nvs_read_string (key_table[mqtt_config_list], (config->pingBackupServer) ,sizeof (config->pingBackupServer));
                // memcpy (config_infor_now.pingBackupServer, string_value, strlen ((char*)string_value));
            }
        break;
    case INT_TYPE:
        if (mqtt_config_list == CHARG_INTERVAL)
        {
            internal_flash_nvs_get_u16(key_table[mqtt_config_list], &(config->charg_interval));
            // config_infor_now.charg_interval = int_value;
        }
        else if (mqtt_config_list == UNCHARG_INTERVAL)
        {
            internal_flash_nvs_get_u16(key_table[mqtt_config_list], &(config->uncharg_interval));
            // config_infor_now.uncharg_interval = int_value;
        } 
        else if (mqtt_config_list == SMOKE_WAKE)
        {
            internal_flash_nvs_get_u16(key_table[mqtt_config_list], &(config->smokeSensorWakeupInterval));
            // config_infor_now.smokeSensorWakeupInterval = int_value;
        } 
        else if (mqtt_config_list == SMOKE_HEARTBEAT)
        {
            internal_flash_nvs_get_u16(key_table[mqtt_config_list], &(config->smokeSensorHeartbeatInterval));
            // config_infor_now.smokeSensorHeartbeatInterval = int_value;
        }
        else if (mqtt_config_list == SMOKE_THRESH)
        {
            internal_flash_nvs_get_u16(key_table[mqtt_config_list], &(config->smokeSensorThresHole));
            // config_infor_now.smokeSensorThresHole = int_value;
        }
        else if (mqtt_config_list == TEMPER_WAKE)
        {
            internal_flash_nvs_get_u16(key_table[mqtt_config_list], &(config->tempSensorWakeupInterval));
            // config_infor_now.tempSensorWakeupInterval = int_value;
        }
        else if (mqtt_config_list == TEMPER_HEARTBEAT)
        {
            internal_flash_nvs_get_u16(key_table[mqtt_config_list], &(config->tempSensorHeartbeatInterval));
            // config_infor_now.tempSensorHeartbeatInterval = int_value;
        }
        else if (mqtt_config_list == TEMPER_THRESH)
        {
            internal_flash_nvs_get_u16(key_table[mqtt_config_list], &(config->tempSensorHeartbeatInterval));
            // config_infor_now.tempThresHold = int_value;
        }
        break;
    case BOOL_TYPE:
        if (mqtt_config_list == SYNC_ALARM)
        {
            internal_flash_nvs_get_u8 (key_table[mqtt_config_list],  &(config->syncAlarm));
        }
        else if (mqtt_config_list == BUZZ_EN)
        {
            internal_flash_nvs_get_u8 (key_table[mqtt_config_list],  &(config->buzzerEnable));
        }
        break;
    default:
        break;
    }
}

