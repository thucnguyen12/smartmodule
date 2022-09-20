set(srcs "src/app_time_sync.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS include)
