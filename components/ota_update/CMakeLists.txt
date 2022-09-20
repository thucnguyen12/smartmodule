set(srcs "src/app_ota.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS include)
