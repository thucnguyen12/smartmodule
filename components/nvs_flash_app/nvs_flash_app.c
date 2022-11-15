
#include "esp_log.h"
#include "esp_err.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "nvs_flash_app.h"
static const char* TAG = "nvs_flash_app";
static int32_t m_err_code = ESP_OK;




// char* key_table [MAX_CONFIG_HANDLE] = {
//     "topicHeader",
//     "mqttAddress",
//     "mqttUserName",
//     "mqttPassword",
//     "chargingInterval",
//     "unchargeInterval",
//     "userPhoneNumber1",
//     "userPhoneNumber2",
//     "userPhoneNumber3",
//     "buzzerEnable",
//     "syncAlarm",
//     "networkAddress",
//     "smokeSensorWakeupInterval",
//     "smokeSensorHeartbeatInterval",
//     "smokeSensorThresHole",
//     "tempSensorWakeupInterval",
//     "tempSensorHeartbeatInterval",
//     "tempThreshHold",
//     "httpDnsName",
//     "httpUsername",
//     "httpDnsPass", 
//     "httpDnsPort",
//     "wifiname",
//     "wifipass",
//     "wifiDisable",
//     "pingMainServer",
//     "pingBackupServer",
//     "inputActiveLevel"
// };

static void process_default_config_data (type_of_mqtt_data_t data_type, mqtt_config_list mqtt_config_list)
{
    static char string_key[32];
    make_key_to_store_type_in_nvs (string_key, mqtt_config_list);
    switch (data_type)
    {
    case STRING_TYPE:
        if (1)
        {
            if (mqtt_config_list == TOPIC_HDR)
            {
                //memcpy (config_infor_now.topic_hr, "SmartModule", strlen ("SmartModule"));
                internal_flash_nvs_write_string (string_key, "SmartModule");
            }
            else if (mqtt_config_list == MQTT_ADDR)
            {
                //memcpy (config_infor_now.mqtt_add, "14.248.80.124", strlen ("14.248.80.124"));
                internal_flash_nvs_write_string (string_key, "14.248.80.124");
            }
            else if (mqtt_config_list == MQTT_USER)
            {
                //memcpy (config_infor_now.mqtt_user, "FireSafe", strlen ("FireSafe"));
                internal_flash_nvs_write_string (string_key, "FireSafe");
            }
            else if (mqtt_config_list == MQTT_PW)
            {
                //memcpy (config_infor_now.mqtt_pass, "FireSafe1929283", strlen ("FireSafe1929283"));
                internal_flash_nvs_write_string (string_key, "FireSafe1929283");
            }
            else if (mqtt_config_list == PHONE_NUM_1)
            {
                //memcpy (config_infor_now.userPhoneNumber1, "0000000000", 10);
                internal_flash_nvs_write_string (string_key, "0000000000");
            }
            else if (mqtt_config_list == PHONE_NUM_2)
            {
                //memcpy (config_infor_now.userPhoneNumber2, "0000000000", 10);
                internal_flash_nvs_write_string (string_key, "0000000000");
            }
            else if (mqtt_config_list == PHONE_NUM_3)
            {
                //memcpy (config_infor_now.userPhoneNumber3, "0000000000", 10);
                internal_flash_nvs_write_string (string_key, "0000000000");
            }
            else if (mqtt_config_list == DNS_NAME)
            {
                //memcpy (config_infor_now.httpDnsName, "btwork.bytechjsc.vn/api/server-info", strlen ("btwork.bytechjsc.vn/api/server-info"));
                internal_flash_nvs_write_string (string_key, "btwork.bytechjsc.vn/api/server-info");
            }
            else if (mqtt_config_list == DNS_USER)
            {
                //memcpy (config_infor_now.httpUsername, "esp32_smart_module", strlen ("esp32_smart_module"));
                internal_flash_nvs_write_string (string_key, "btwork.bytechjsc.vn/api/server-info");
            }
            else if (mqtt_config_list == DNS_PASS)
            {
                //memcpy (config_infor_now.httpDnsPass, "ZmlyZS1zYWZlOmFiY2QxMjM0", strlen ("ZmlyZS1zYWZlOmFiY2QxMjM0"));
                internal_flash_nvs_write_string (string_key, "ZmlyZS1zYWZlOmFiY2QxMjM0");
            }
            else if (mqtt_config_list == WIFI_NAME)
            {
                //memcpy (config_infor_now.wifiname, "BYTECH_T3", strlen ("BYTECH_T3"));
                internal_flash_nvs_write_string (string_key, "BYTECH_T3");
            }
            else if (mqtt_config_list == WIFI_PASS)
            {
                //memcpy (config_infor_now.wifipass, "bytech@2020", strlen ("bytech@2020"));
                internal_flash_nvs_write_string (string_key, "bytech@2020");
            }
            else if (mqtt_config_list == PING_MAIN_SERVER)
            {
                //memcpy (config_infor_now.pingMainServer, "www.google.com", strlen ("www.google.com"));
                internal_flash_nvs_write_string (string_key, "www.google.com");
            }
            else if (mqtt_config_list == PING_BACKUP_SERVER)
            {
                //memcpy (config_infor_now.pingBackupServer, "www.google.com", strlen ("www.google.com"));
                internal_flash_nvs_write_string (string_key, "www.google.com");
            }
            // ghi data vao flash
            
        }
        break;
    case INT_TYPE:
        if (mqtt_config_list == CHARG_INTERVAL)
        {
            //config_infor_now.charg_interval = int_value;
            internal_flash_nvs_write_u16 (string_key, 1000);
        }
        
        if (mqtt_config_list == UNCHARG_INTERVAL)
        {
            //config_infor_now.uncharg_interval = int_value;
            internal_flash_nvs_write_u16 (string_key, 3000);
        }
        
        if (mqtt_config_list == SMOKE_WAKE)
        {
            //config_infor_now.smokeSensorWakeupInterval = int_value;
            internal_flash_nvs_write_u16 (string_key, 2000);
        }
        
        if (mqtt_config_list == SMOKE_HEARTBEAT)
        {
            //config_infor_now.smokeSensorHeartbeatInterval = int_value;
            internal_flash_nvs_write_u16 (string_key, 3000);
        }
        
        if (mqtt_config_list == SMOKE_THRESH)
        {
            ///config_infor_now.smokeSensorThresHole = int_value;
            internal_flash_nvs_write_u16 (string_key, 20);
        }
        
        if (mqtt_config_list == TEMPER_WAKE)
        {
            //config_infor_now.tempSensorWakeupInterval = int_value;
            internal_flash_nvs_write_u16 (string_key, 2000);
        }
        
        if (mqtt_config_list == TEMPER_HEARTBEAT)
        {
            //config_infor_now.tempSensorHeartbeatInterval = int_value;
            internal_flash_nvs_write_u16 (string_key, 3000);
        }

        if (mqtt_config_list == TEMPER_THRESH)
        {
            //config_infor_now.tempThresHold = int_value;
            internal_flash_nvs_write_u16 (string_key, 15);
        }

        
        break;
    case BOOL_TYPE:
        if (mqtt_config_list == BUZZ_EN)
        {
            //config_infor_now.buzzerEnable = bool_value;
            internal_flash_nvs_write_u8 (string_key, 1);
        }
        else if (mqtt_config_list == SYNC_ALARM)
        {
            //config_infor_now.syncAlarm = bool_value;
            internal_flash_nvs_write_u8 (string_key, 0);
        }
        
        break;
    default:
        break;
    }
}

void internal_flash_store_default_config(void)
{
    type_of_mqtt_data_t type_of_process_data;
    for (mqtt_config_list i = 0; i < MAX_CONFIG_HANDLE; i++)
    {
        type_of_process_data = get_type_of_data (i);
        process_default_config_data (type_of_process_data, i);
    }
}

type_of_mqtt_data_t get_type_of_data (mqtt_config_list mqtt_config_element)
{
    type_of_mqtt_data_t type;
    
    // if (strstr (key, "buzzerEnable") || strstr (key, "syncAlarm"))
    if ((mqtt_config_element == BUZZ_EN) || (mqtt_config_element == SYNC_ALARM))
    {
        type = BOOL_TYPE;
    }
    else if ((mqtt_config_element == TOPIC_HDR)
            || (mqtt_config_element == MQTT_ADDR)
            || (mqtt_config_element == MQTT_USER)
            || (mqtt_config_element == MQTT_PW)
            || (mqtt_config_element == NETWORK_ADDR)
            || (mqtt_config_element == PHONE_NUM_1)
            || (mqtt_config_element == PHONE_NUM_2)
            || (mqtt_config_element == PHONE_NUM_3)
            || (mqtt_config_element == DNS_NAME)
            || (mqtt_config_element == DNS_USER)
            || (mqtt_config_element == DNS_PASS)
            || (mqtt_config_element == WIFI_NAME)
            || (mqtt_config_element == WIFI_PASS)
            || (mqtt_config_element == PING_MAIN_SERVER)
            || (mqtt_config_element == PING_BACKUP_SERVER)
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
    sprintf(out_string, "%02x%02x%02x%02x%02x%02x", device_mac[0],
                                                        device_mac[1],
                                                        device_mac[2],
                                                        device_mac[3],
                                                        device_mac[4],
                                                        device_mac[5]);
}

#define UNICAST_ADD_STORE_KEY "u_addr_key"
#define UNICAST_ADDR_BASE 0X1000



uint16_t get_and_store_new_unicast_addr_now (void)
{
    uint16_t unicast_addr = 0;
    esp_err_t err = internal_flash_nvs_get_u16(UNICAST_ADD_STORE_KEY, &unicast_addr);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGI (TAG, "CREAT BASE ADDR");
        unicast_addr = UNICAST_ADDR_BASE;
        internal_flash_nvs_write_u16 (UNICAST_ADD_STORE_KEY, unicast_addr); 

    }
    else if (err == ESP_OK)
    {
        ESP_LOGI (TAG, "GET UNCAST ADDR AND INCREASE IT");
        unicast_addr += 4;//dinh nghia lai ve khoang cach danh dia chi
        internal_flash_nvs_write_u16 (UNICAST_ADD_STORE_KEY, unicast_addr);
    }
    else
    {
        ESP_LOGE (TAG, "GET UNICAST ADDR GOT ERR: %s", esp_err_to_name(err));
    }
    return unicast_addr;
}

esp_err_t find_and_write_into_mac_key_space (uint16_t* unicast_addr, char* mac_key)
{
    nvs_handle_t my_handle;
    esp_err_t ret = ESP_OK;
    //node_sensor_data_t data_sensor;
    uint16_t unicast_addr_temp = 0;
    //OPEN FLASH
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    ret |= err; 
    if (err == ESP_OK) 
    {
        //err = nvs_get_blob (my_handle, mac_key, &data_sensor, byte_write);
        err = internal_flash_nvs_get_u16 (mac_key, unicast_addr);
        switch (err) {
        case ESP_OK:
            //unicast_addr = data_sensor.unicast_add;
            ESP_LOGI(TAG,"Done\n");
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI(TAG,"The value is not initialized yet! Creat new unicast addr\n");
            unicast_addr_temp = get_and_store_new_unicast_addr_now();

            if (unicast_addr_temp)
            {
                // data_write->unicast_add = unicast_addr;
                // err = nvs_set_blob (my_handle, mac_key, data_write, sizeof (node_sensor_data_t));

                *unicast_addr = unicast_addr_temp;
                err = internal_flash_nvs_write_u16 (mac_key, unicast_addr_temp);
                ESP_LOGI (TAG, "err write uncast add :%s", esp_err_to_name (err));
                err = internal_flash_nvs_get_u16 (mac_key, unicast_addr);
                ESP_LOGI (TAG, "err get uncast add :%s", esp_err_to_name (err));
            }
            else
            {
                ESP_LOGE (TAG, "get new unicast addr err");
            }
            break;
        // case ESP_ERR_NVS_KEYS_NOT_INITIALIZED:
        //     ESP_LOGI("The value is not initialized yet!\n");
        //     break;
        default :
            ESP_LOGI(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
            break;
        }
        nvs_close(my_handle);
    }
    else
    {
        ESP_LOGI(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    return err;
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
        err = nvs_get_blob (my_handle, key, data_read, (size_t *)byte_read);
        //ESP_LOGI(TAG, "Error (%s) get NVS handle!\n", esp_err_to_name(err));
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
        ESP_LOGI(TAG, "error: %d", err);
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

uint32_t read_config_data_from_flash (info_config_t* config, type_of_mqtt_data_t data_type, mqtt_config_list mqtt_config_list)
{
    esp_err_t err = ESP_OK;
    switch (data_type)
    {
    case STRING_TYPE:
        /* code */
        if (mqtt_config_list == TOPIC_HDR)
            {
                err = internal_flash_nvs_read_string (INTERNAL_FLASH_NVS_KEY_TOPIC_HEADER, (config->topic_hr) ,sizeof (config->topic_hr));
                // memcpy (config_infor_now.topic_hr, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == MQTT_ADDR)
            {
                err = internal_flash_nvs_read_string (INTERNAL_FLASH_NVS_KEY_MQTT_ADDR, (config->mqtt_add) ,sizeof (config->mqtt_add));
                // memcpy (config_infor_now.mqtt_add, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == MQTT_USER)
            {
                err = internal_flash_nvs_read_string (INTERNAL_FLASH_NVS_KEY_MQTT_USERNAME, (config->mqtt_user) ,sizeof (config->mqtt_user));
                // memcpy (config_infor_now.mqtt_pass, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == MQTT_PW)
            {
                err = internal_flash_nvs_read_string (INTERNAL_FLASH_NVS_KEY_MQTT_PASSWORD, (config->mqtt_pass) ,sizeof (config->mqtt_pass));
                // memcpy (config_infor_now.mqtt_pass, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == PHONE_NUM_1)
            {
                err = internal_flash_nvs_read_string (INTERNAL_FLASH_NVS_KEY_PHONE_1, (config->userPhoneNumber1) ,sizeof (config->userPhoneNumber1));
                // memcpy (config_infor_now.userPhoneNumber1, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == PHONE_NUM_2)
            {
                err = internal_flash_nvs_read_string (INTERNAL_FLASH_NVS_KEY_PHONE_2, (config->userPhoneNumber2) ,sizeof (config->userPhoneNumber2));
                // memcpy (config_infor_now.userPhoneNumber2, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == PHONE_NUM_3)
            {
                err = internal_flash_nvs_read_string (INTERNAL_FLASH_NVS_KEY_PHONE_3, (config->userPhoneNumber3) ,sizeof (config->userPhoneNumber3));
                // memcpy (config_infor_now.userPhoneNumber3, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == NETWORK_ADDR)
            {
                err = internal_flash_nvs_read_string (INTERNAL_FLASH_NVS_KEY_NETWORK_ADDR, (config->userPhoneNumber3) ,sizeof (config->userPhoneNumber3));
                // memcpy (config_infor_now.userPhoneNumber3, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == DNS_NAME)
            {
                err = internal_flash_nvs_read_string (INTERNAL_FLASH_NVS_KEY_DNS_NAME, (config->httpDnsName) ,sizeof (config->httpDnsName));
                // memcpy (config_infor_now.httpDnsName, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == DNS_USER)
            {
                err = internal_flash_nvs_read_string (INTERNAL_FLASH_NVS_KEY_DNS_USER, (config->httpUsername) ,sizeof (config->httpUsername));
                // memcpy (config_infor_now.httpUsername, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == DNS_PASS)
            {
                err = internal_flash_nvs_read_string (INTERNAL_FLASH_NVS_KEY_DNS_PASS, (config->httpDnsPass) ,sizeof (config->httpDnsPass));
                // memcpy (config_infor_now.httpUsername, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == WIFI_NAME)
            {
                err = internal_flash_nvs_read_string (INTERNAL_FLASH_NVS_KEY_WIFI_NAME, (config->wifiname) ,sizeof (config->wifiname));
                // memcpy (config_infor_now.httpUsername, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == WIFI_PASS)
            {
                err = internal_flash_nvs_read_string (INTERNAL_FLASH_NVS_KEY_WIFI_PASS, (config->wifipass) ,sizeof (config->wifipass));
                // memcpy (config_infor_now.wifipass, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == PING_MAIN_SERVER)
            {
                err = internal_flash_nvs_read_string (INTERNAL_FLASH_NVS_KEY_PING_MAIN_SERVER, (config->pingMainServer) ,sizeof (config->pingMainServer));
                // memcpy (config_infor_now.pingMainServer, string_value, strlen ((char*)string_value));
            }
            else if (mqtt_config_list == PING_BACKUP_SERVER)
            {
                err = internal_flash_nvs_read_string (INTERNAL_FLASH_NVS_KEY_PING_BACKUP_SERVER, (config->pingBackupServer) ,sizeof (config->pingBackupServer));
                // memcpy (config_infor_now.pingBackupServer, string_value, strlen ((char*)string_value));
            }
        break;
    case INT_TYPE:
        if (mqtt_config_list == CHARG_INTERVAL)
        {
            err = internal_flash_nvs_get_u16(INTERNAL_FLASH_NVS_KEY_CHARG_INTERVAL, (uint16_t*)&(config->charg_interval));
            // config_infor_now.charg_interval = int_value;
        }
        else if (mqtt_config_list == UNCHARG_INTERVAL)
        {
            err = internal_flash_nvs_get_u16(INTERNAL_FLASH_NVS_KEY_UNCHARG_INTERVAL, (uint16_t*)&(config->uncharg_interval));
            // config_infor_now.uncharg_interval = int_value;
        } 
        else if (mqtt_config_list == SMOKE_WAKE)
        {
            err = internal_flash_nvs_get_u16(INTERNAL_FLASH_NVS_KEY_SMOKE_WAKE, (uint16_t*)&(config->smokeSensorWakeupInterval));
            // config_infor_now.smokeSensorWakeupInterval = int_value;
        } 
        else if (mqtt_config_list == SMOKE_HEARTBEAT)
        {
            err = internal_flash_nvs_get_u16(INTERNAL_FLASH_NVS_KEY_SMOKE_HEARTBEAT, (uint16_t*)&(config->smokeSensorHeartbeatInterval));
            // config_infor_now.smokeSensorHeartbeatInterval = int_value;
        }
        else if (mqtt_config_list == SMOKE_THRESH)
        {
            err = internal_flash_nvs_get_u16(INTERNAL_FLASH_NVS_KEY_SMOKE_THRESH, (uint16_t*)&(config->smokeSensorThresHole));
            // config_infor_now.smokeSensorThresHole = int_value;
        }
        else if (mqtt_config_list == TEMPER_WAKE)
        {
            err = internal_flash_nvs_get_u16(INTERNAL_FLASH_NVS_KEY_TEMP_WAKE, (uint16_t*)&(config->tempSensorWakeupInterval));
            // config_infor_now.tempSensorWakeupInterval = int_value;
        }
        else if (mqtt_config_list == TEMPER_HEARTBEAT)
        {
            err = internal_flash_nvs_get_u16(INTERNAL_FLASH_NVS_KEY_TEMP_HEARTBEAT, (uint16_t*)&(config->tempSensorHeartbeatInterval));
            // config_infor_now.tempSensorHeartbeatInterval = int_value;
        }
        else if (mqtt_config_list == TEMPER_THRESH)
        {
            err = internal_flash_nvs_get_u16(INTERNAL_FLASH_NVS_KEY_TEMP_THRESH, (uint16_t*)&(config->tempSensorHeartbeatInterval));
            // config_infor_now.tempThresHold = int_value;
        }
        break;
    case BOOL_TYPE:
        if (mqtt_config_list == SYNC_ALARM)
        {
            err = internal_flash_nvs_get_u8 (INTERNAL_FLASH_NVS_KEY_SYNC_ALARM,  (uint8_t *)&(config->syncAlarm));
        }
        else if (mqtt_config_list == BUZZ_EN)
        {
            err = internal_flash_nvs_get_u8 (INTERNAL_FLASH_NVS_KEY_BUZZ_EN,  (uint8_t *)&(config->buzzerEnable));
        }
        break;
    default:
        break;
    }
    return err;
}

void make_key_from_mqtt_list_type (char* str_out, mqtt_config_list mqtt_config_element)
{
    if (mqtt_config_element == TOPIC_HDR)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_TOPIC_HEADER);
    }
    else if (mqtt_config_element == MQTT_ADDR)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_MQTT_ADDR);
    }
    else if (mqtt_config_element == MQTT_USER)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_MQTT_USERNAME);
    }
    else if (mqtt_config_element == MQTT_PW)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_MQTT_PASSWORD);
    }
    else if (mqtt_config_element == CHARG_INTERVAL)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_CHARG_INTERVAL);
    }
    else if (mqtt_config_element == UNCHARG_INTERVAL)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_UNCHARG_INTERVAL);
    }
    else if (mqtt_config_element == PHONE_NUM_1)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_PHONE_1);
    }
    else if (mqtt_config_element == PHONE_NUM_2)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_PHONE_2);
    }
    else if (mqtt_config_element == PHONE_NUM_3)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_PHONE_3);
    }
    else if (mqtt_config_element == BUZZ_EN)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_BUZZ_EN);
    }
    else if (mqtt_config_element == SYNC_ALARM)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_SYNC_ALARM);
    }
    else if (mqtt_config_element == NETWORK_ADDR)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_NETWORK_ADDR);
    }
    else if (mqtt_config_element == SMOKE_WAKE)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_SMOKE_WAKE);
    }
    else if (mqtt_config_element == SMOKE_HEARTBEAT)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_SMOKE_HEARTBEAT);
    }
    else if (mqtt_config_element == SMOKE_THRESH)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_SMOKE_THRESH);
    }
    else if (mqtt_config_element == TEMPER_WAKE)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_TEMP_WAKE);
    }
    else if (mqtt_config_element == TEMPER_HEARTBEAT)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_TEMP_HEARTBEAT);
    }
    else if (mqtt_config_element == TEMPER_THRESH)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_TEMP_THRESH);
    }
    else if (mqtt_config_element == DNS_NAME)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_DNS_NAME);
    }
    else if (mqtt_config_element == DNS_USER)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_DNS_USER);
    }
    else if (mqtt_config_element == DNS_PASS)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_DNS_PASS);
    }
    else if (mqtt_config_element == DNS_PORT)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_DNS_PORT);
    }
    else if (mqtt_config_element == WIFI_NAME)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_WIFI_NAME);
    }
    else if (mqtt_config_element == WIFI_PASS)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_WIFI_PASS);
    }
    else if (mqtt_config_element == WIFI_DIS)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_WIFI_DIS);
    }
    else if (mqtt_config_element == PING_MAIN_SERVER)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_PING_MAIN_SERVER);
    }
    else if (mqtt_config_element == PING_BACKUP_SERVER)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_PING_BACKUP_SERVER);
    }
    else if (mqtt_config_element == INPUT_LEVEL)
    {
        sprintf (str_out,  MQTT_SEVER_KEY_INPUT_ACTIVE_LEVEL);
    }
    return;
}
void make_key_to_store_type_in_nvs (char* str_out, mqtt_config_list mqtt_config_element)
{
    if (mqtt_config_element == TOPIC_HDR)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_TOPIC_HEADER);
    }
    else if (mqtt_config_element == MQTT_ADDR)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_MQTT_ADDR);
    }
    else if (mqtt_config_element == MQTT_USER)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_MQTT_USERNAME);
    }
    else if (mqtt_config_element == MQTT_PW)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_MQTT_PASSWORD);
    }
    else if (mqtt_config_element == CHARG_INTERVAL)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_CHARG_INTERVAL);
    }
    else if (mqtt_config_element == UNCHARG_INTERVAL)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_UNCHARG_INTERVAL);
    }
    else if (mqtt_config_element == PHONE_NUM_1)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_PHONE_1);
    }
    else if (mqtt_config_element == PHONE_NUM_2)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_PHONE_2);
    }
    else if (mqtt_config_element == PHONE_NUM_3)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_PHONE_3);
    }
    else if (mqtt_config_element == BUZZ_EN)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_BUZZ_EN);
    }
    else if (mqtt_config_element == SYNC_ALARM)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_SYNC_ALARM);
    }
    else if (mqtt_config_element == NETWORK_ADDR)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_NETWORK_ADDR);
    }
    else if (mqtt_config_element == SMOKE_WAKE)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_SMOKE_WAKE);
    }
    else if (mqtt_config_element == SMOKE_HEARTBEAT)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_SMOKE_HEARTBEAT);
    }
    else if (mqtt_config_element == SMOKE_THRESH)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_SMOKE_THRESH);
    }
    else if (mqtt_config_element == TEMPER_WAKE)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_TEMP_WAKE);
    }
    else if (mqtt_config_element == TEMPER_HEARTBEAT)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_TEMP_HEARTBEAT);
    }
    else if (mqtt_config_element == TEMPER_THRESH)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_TEMP_THRESH);
    }
    else if (mqtt_config_element == DNS_NAME)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_DNS_NAME);
    }
    else if (mqtt_config_element == DNS_USER)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_DNS_USER);
    }
    else if (mqtt_config_element == DNS_PASS)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_DNS_PASS);
    }
    else if (mqtt_config_element == DNS_PORT)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_DNS_PORT);
    }
    else if (mqtt_config_element == WIFI_NAME)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_WIFI_NAME);
    }
    else if (mqtt_config_element == WIFI_PASS)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_WIFI_PASS);
    }
    else if (mqtt_config_element == WIFI_DIS)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_WIFI_DIS);
    }
    else if (mqtt_config_element == PING_MAIN_SERVER)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_PING_MAIN_SERVER);
    }
    else if (mqtt_config_element == PING_BACKUP_SERVER)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_PING_BACKUP_SERVER);
    }
    else if (mqtt_config_element == INPUT_LEVEL)
    {
        sprintf (str_out,  INTERNAL_FLASH_NVS_KEY_INPUT_ACTIVE_LEVEL);
    }
    return;
}