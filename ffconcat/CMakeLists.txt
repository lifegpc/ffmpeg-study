cmake_minimum_required(VERSION 3.0)

project(ffconcat)

if (MSVC)
    add_compile_options(/utf-8)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

find_package(AVFORMAT 59 REQUIRED)
find_package(AVCODEC 59 REQUIRED)
find_package(AVUTIL 57 REQUIRED)

include_directories(${AVFORMAT_INCLUDE_DIRS})

include(CheckIncludeFiles)

CHECK_INCLUDE_FILES(getopt.h HAVE_GETOPT_H)
if (NOT HAVE_GETOPT_H)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../getopt" "${CMAKE_BINARY_DIR}/getopt")
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../getopt")
endif()
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../utils" "${CMAKE_BINARY_DIR}/utils")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../utils")

add_executable(ffconcat ffconcat.h ffconcat.cpp main.cpp)
if (TARGET getopt)
    target_link_libraries(ffconcat getopt)
endif()
target_link_libraries(ffconcat AVFORMAT::AVFORMAT)
target_link_libraries(ffconcat AVUTIL::AVUTIL)
target_link_libraries(ffconcat AVCODEC::AVCODEC)
target_link_libraries(ffconcat utils)
