cmake_minimum_required(VERSION 3.0)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(utils STATIC list_pointer.h wchar_util.h wchar_util.cpp)
