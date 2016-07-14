include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/src/third_party)

set(HSTAR_LIBS
	hstar
	# crypto ssl
)

add_library(hstar STATIC engine.c)

add_executable(hstar_cli cli.c)
target_link_libraries(hstar_cli ${HSTAR_LIBS})

add_executable(engine_test engine_test.c)
target_link_libraries(engine_test ${HSTAR_LIBS})
add_test(NAME engine COMMAND engine_test)

add_subdirectory(third_party)
