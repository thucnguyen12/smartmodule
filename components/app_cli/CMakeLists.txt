set(srcs "app_cli.c"
	 "app_shell.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS ".")
