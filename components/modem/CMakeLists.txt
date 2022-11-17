set(srcs "src/esp_modem.c"
        "src/esp_modem_dce_service"
        "src/esp_modem_netif.c"
        "src/esp_modem_compat.c"
#        "src/sim800.c"
#        "src/sim7600.c"
#       "src/fibocom_l610.c"
        "src/hw_power.c"
        "src/ec2x.c"
        "src/gsm_hardware.c"
        "src/gsm_utilities.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS include
                    PRIV_INCLUDE_DIRS private_include
                    REQUIRES driver nvs_flash_app app_uart)
