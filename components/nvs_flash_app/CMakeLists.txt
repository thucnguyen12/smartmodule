set(srcs "nvs_flash_app.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash log)
