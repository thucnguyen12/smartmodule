idf_component_register(SRCS "pppos_client_main.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver mqtt modem nvs_flash app_update esp_http_client esp_https_ota tinyusb app_uart app_wifi ota_update sntp_service min_protocol)
