set(srcs "src/app_uart.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS include)
