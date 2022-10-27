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


void build_string_from_MAC (uint8_t* device_mac, char * out_string);
uint16_t get_and_store_new_unicast_addr_now (void);
uint16_t find_and_write_into_mac_key_space (node_sensor_data_t* data_write, size_t* byte_write, char* mac);
void init_nvs_flash(void );
int32_t write_data_to_flash(void *data_write, size_t byte_write, const char* key);
esp_err_t read_data_from_flash (void *data_read, uint16_t* byte_read, const char* key);

#endif