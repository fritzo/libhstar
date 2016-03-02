set(HSTAR_LIBS
	hstar
	# crypto ssl
)

add_library(hstar STATIC
	engine.c
)

add_executable(hstar_cli
	cli.c
)

target_link_libraries(hstar_cli ${HSTAR_LIBS})
