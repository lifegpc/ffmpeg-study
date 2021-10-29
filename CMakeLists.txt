cmake_minimum_required(VERSION 3.0)

project(ffmpeg-study)

option(ENABLE_RSSBOTLIB "Build RSSBotLib" OFF)

if (MSVC)
    add_compile_options(/utf-8)
endif()

include(CheckIncludeFiles)

CHECK_INCLUDE_FILES(getopt.h HAVE_GETOPT_H)
if (NOT HAVE_GETOPT_H)
    add_subdirectory(getopt)
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/getopt")
endif()

add_subdirectory(utils)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/utils")

add_subdirectory(enm4a)
add_subdirectory(ffconcat)
if (ENABLE_RSSBOTLIB)
    add_subdirectory(rssbotlib)
endif()
