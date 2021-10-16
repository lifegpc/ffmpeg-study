cmake_minimum_required(VERSION 3.0)

project(enm4a)

if (MSVC)
    add_compile_options(/utf-8)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

find_package(AVFORMAT 59 REQUIRED)
find_package(AVCODEC 59 REQUIRED)
find_package(AVUTIL 57 REQUIRED)

include_directories(${AVFORMAT_INCLUDE_DIRS})

include(CheckIncludeFiles)
include(CheckSymbolExists)
include(GNUInstallDirs)
include(ENM4A_VERSION)
if (WIN32)
    check_symbol_exists(printf_s "stdio.h" HAVE_PRINTF_S)
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/enm4a_config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/enm4a_config.h")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

CHECK_INCLUDE_FILES(getopt.h HAVE_GETOPT_H)
if (NOT HAVE_GETOPT_H)
    if (NOT TARGET getopt)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../getopt" "${CMAKE_BINARY_DIR}/getopt")
        include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../getopt")
    endif()
endif()
if (NOT TARGET utils)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../utils" "${CMAKE_BINARY_DIR}/utils")
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../utils")
endif()

add_custom_target(enm4a_version
    ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_SOURCE_DIR}/ENM4A_VERSION.cmake"
)

if (WIN32)
    set(ENM4A_RC "${CMAKE_CURRENT_BINARY_DIR}/enm4a.rc")
endif()

add_executable(enm4a enm4a.h enm4a.c main.cpp ${ENM4A_RC})
add_dependencies(enm4a enm4a_version)
target_compile_definitions(enm4a PRIVATE HAVE_ENM4A_CONFIG_H)
if (TARGET getopt)
    target_link_libraries(enm4a getopt)
endif()
target_link_libraries(enm4a AVFORMAT::AVFORMAT)
target_link_libraries(enm4a AVUTIL::AVUTIL)
target_link_libraries(enm4a AVCODEC::AVCODEC)
target_link_libraries(enm4a utils)
install(TARGETS enm4a)
