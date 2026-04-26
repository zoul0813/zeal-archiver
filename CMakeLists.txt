cmake_minimum_required(VERSION 3.16)

set(ZOS_TOOLCHAIN sdcc)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
include($ENV{ZOS_PATH}/cmake/zos_init.cmake)

project(zar C)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

zos_use(coreutils)
find_package(COREUTILS REQUIRED)

add_library(zar STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/libsrc/zar.c
)

target_compile_options(zar PRIVATE
    --nostdlib
    --no-xinit-opt
)

target_link_libraries(zar PUBLIC core)
target_include_directories(zar PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(zar PROPERTIES
    OUTPUT_NAME zar
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

add_executable(zar_cli
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stdutils.c
)

target_compile_options(zar_cli PRIVATE
    --nostdlib
    --no-xinit-opt
)

target_include_directories(zar_cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

zos_link_libraries(zar_cli PRIVATE zar core)
set_target_properties(zar_cli PROPERTIES
    OUTPUT_NAME zar
)
ihx_to_bin(zar_cli "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/zar.bin")
