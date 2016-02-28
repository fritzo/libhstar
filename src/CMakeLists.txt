include_directories(${CMAKE_SOURCE_DIR}/include)

set(HSTAR_LIBS
	hstar_util
	# crypto ssl
)
add_executable(hstar_main
	hstar_main.c
)

# add_subdirectory(third_party)
# add_subdirectory(util)
