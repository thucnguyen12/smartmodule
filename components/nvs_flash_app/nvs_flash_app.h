#ifndef NSV_FLASH_APP
#define NSV_FLASH_APP

#include "nvs_flash.h"
#include "nvs.h"

void init_nvs_flash(void );
int32_t write_data_to_flash(void *data_write, size_t byte_write, const char* key);
esp_err_t read_data_from_flash (void *data_read, uint16_t byte_read, const char* key);

#endif