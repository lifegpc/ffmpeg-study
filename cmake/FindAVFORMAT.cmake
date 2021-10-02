find_package(PkgConfig)
if (PkgConfig_FOUND)
    pkg_check_modules(PC_AVFORMAT QUIET libavformat)
endif()

if (PC_AVFORMAT_FOUND)
    set(AVFORMAT_FOUND TRUE)
    set(AVFORMAT_VERSION ${PC_AVFORMAT_VERSION})
    set(AVFORMAT_VERSION_STRING ${PC_AVFORMAT_STRING})
    if (USE_STATIC_LIBS)
        set(AVFORMAT_LIBRARYS ${PC_AVFORMAT_STATIC_LNIK_LIBRARIES})
        set(AVFORMAT_INCLUDE_DIRS ${PC_AVFORMAT_STATIC_INCLUDE_DIRS})
    else()
        set(AVFORMAT_LIBRARYS ${PC_AVFORMAT_LINK_LIBRARIES})
        set(AVFORMAT_INCLUDE_DIRS ${PC_AVFORMAT_INCLUDE_DIRS})
    endif()
    if (NOT TARGET AVFORMAT::AVFORMAT)
        add_library(AVFORMAT::AVFORMAT UNKNOWN IMPORTED)
        set_target_properties(AVFORMAT::AVFORMAT PROPERTIES
            IMPORTED_LOCATION "${AVFORMAT_LIBRARYS}"
            INTERFACE_COMPILE_OPTIONS "${PC_AVFORMAT_CFLAGS}"
            INTERFACE_INCLUDE_DIRECTORIES "${AVFORMAT_INCLUDE_DIRS}"
        )
    endif()
else()
    message(FATAL_ERROR "failed.")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AVFORMAT
    FOUND_VAR AVFORMAT_FOUND
    REQUIRED_VARS
        AVFORMAT_LIBRARYS
        AVFORMAT_INCLUDE_DIRS
    VERSION_VAR AVFORMAT_VERSION
)