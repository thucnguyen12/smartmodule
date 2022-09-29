set(srcs "src/app_uart.c"
	"src/min.c"
	"src/lwrb.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS include
                    REQUIRES min_protocol lwrb)
