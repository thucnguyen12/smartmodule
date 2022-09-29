set(srcs "src/app_ota.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS include
                    REQUIRES app_update esp_http_client esp_https_ota)
