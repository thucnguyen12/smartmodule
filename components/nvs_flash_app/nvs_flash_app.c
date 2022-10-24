#include "nvs_flash_app.h"


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
esp_err_t read_data_from_flash (void *data_read, uint16_t byte_read, const char* key)
{
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        esp_err_t err = nvs_get_blob(my_handle, key, data_read, byte_read);
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
        nvs_close(my_handle);
    }
    //m_err_code = err;
    return err;
}
