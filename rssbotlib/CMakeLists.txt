cmake_minimum_required(VERSION 3.18)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

project(rssbotlib)

if (MSVC)
    add_compile_options(/utf-8)
endif()

find_package(AVFORMAT 58 REQUIRED)
find_package(AVCODEC 58 REQUIRED)
find_package(AVUTIL 56 REQUIRED)
find_package(SWSCALE 5 REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)

include(GNUInstallDirs)

set(RSSBOTLIB_FILES "${CMAKE_CURRENT_SOURCE_DIR}/_rssbotlib.c")
if (WIN32)
    list(APPEND RSSBOTLIB_FILES "${CMAKE_CURRENT_BINARY_DIR}/rssbotlib.rc")
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/rssbotlib.rc" PROPERTIES GENERATED TRUE)
endif()
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/_rssbotlib.c" PROPERTIES GENERATED TRUE)

include_directories(${Python3_INCLUDE_DIR})

if (NOT TARGET utils)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../utils" "${CMAKE_BINARY_DIR}/utils")
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../utils")
endif()

add_custom_target(rssbotlib-gen "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/gen.py")
if (WIN32)
    add_custom_target(rssbotlib-version ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_SOURCE_DIR}/RSSBOTLIB_VERSION.cmake")
endif()
add_library(_rssbotlib SHARED "${RSSBOTLIB_FILES}" videoinfo.c videoinfo.h
ugoira.c ugoira.h zipfile.c zipfile.h tg_thumbnail.c tg_thumbnail.h
warp.c warp.h)
if (WIN32)
    set_target_properties(_rssbotlib PROPERTIES SUFFIX .pyd)
endif()
set_target_properties(_rssbotlib PROPERTIES PREFIX "")
add_dependencies(_rssbotlib rssbotlib-gen)
if (WIN32)
    add_dependencies(_rssbotlib rssbotlib-version)
endif()
target_link_libraries(_rssbotlib Python3::Module)
target_link_libraries(_rssbotlib AVFORMAT::AVFORMAT)
target_link_libraries(_rssbotlib AVCODEC::AVCODEC)
target_link_libraries(_rssbotlib AVUTIL::AVUTIL)
target_link_libraries(_rssbotlib SWSCALE::SWSCALE)
target_link_libraries(_rssbotlib utils)

install(TARGETS _rssbotlib)
