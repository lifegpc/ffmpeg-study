cmake_minimum_required(VERSION 3.17)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

include(CheckSymbolExists)
include(TestStrerrorR)
if (WIN32)
    check_symbol_exists(_access_s io.h HAVE__ACCESS_S)
    check_symbol_exists(_waccess_s io.h HAVE__WACCESS_S)
    check_symbol_exists(strerror_s "string.h" HAVE_STRERROR_S)
    check_symbol_exists(_wcserror_s "string.h" HAVE__WCSERROR_S)
    check_symbol_exists(printf_s "stdio.h" HAVE_PRINTF_S)
    check_symbol_exists(sscanf_s "stdio.h" HAVE_SSCANF_S)
endif()
check_symbol_exists(strerror_r "string.h" HAVE_STRERROR_R)
if (HAVE_STRERROR_R)
    test_strerror_r(HAVE_GNU_STRERROR_R)
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/utils_config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/utils_config.h")

set(SOURCE_FILE
    cfileop.cpp
    cpp2c.cpp
    cstr_util.c
    err.cpp
    fileop.cpp
    wchar_util.cpp
    memfile.c
    cmath.c
)
set(SOURCE_FILE_HEADERS
    cfileop.h
    cpp2c.h
    cstr_util.h
    err.h
    fileop.h
    list_pointer.h
    wchar_util.h
    memfile.h
    cmath.h
)

add_library(utils STATIC ${SOURCE_FILE} ${SOURCE_FILE_HEADERS})
target_compile_definitions(utils PRIVATE HAVE_UTILS_CONFIG_H)
