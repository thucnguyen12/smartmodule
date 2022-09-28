set(srcs "src/app_mqtt.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS include)
