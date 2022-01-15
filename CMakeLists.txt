cmake_minimum_required(VERSION 3.13)
project(radiosonde_decoder C CXX)

file(GLOB_RECURSE SRC "src/*.c" "src/*.h" "src/*.cpp" "src/*.hpp")

add_library(radiosonde_decoder SHARED ${SRC})
target_link_libraries(radiosonde_decoder PRIVATE sdrpp_core)
set_target_properties(radiosonde_decoder PROPERTIES PREFIX "")
target_include_directories(radiosonde_decoder PRIVATE "src/")

set(UNISTALL_TARGET_SAVED "${UNINSTALL_TARGET}")
set(UNINSTALL_TARGET OFF CACHE INTERNAL "")
add_subdirectory("src/decode/sondedump" EXCLUDE_FROM_ALL)
set(UNISTALL_TARGET "${UNINSTALL_TARGET_SAVED}")
target_link_libraries(radiosonde_decoder PRIVATE sondedump_static)

if (MSVC)
	target_compile_options(radiosonde_decoder PRIVATE /O2 /Ob2 $<$<COMPILE_LANGUAGE:CXX>:/std:c++17> /EHsc)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	target_compile_options(radiosonde_decoder PRIVATE -O3 $<$<COMPILE_LANGUAGE:C>:-std=c99> $<$<COMPILE_LANGUAGE:CXX>:-std=c++17> -Wno-unused-command-line-argument -undefined dynamic_lookup)
else ()
	target_compile_options(radiosonde_decoder PRIVATE -O3 -g $<$<COMPILE_LANGUAGE:C>:-std=c99> $<$<COMPILE_LANGUAGE:CXX>:-std=c++17> -Wl,--no-undefined)
endif ()

# Install directives
install(TARGETS radiosonde_decoder DESTINATION lib/sdrpp/plugins)
