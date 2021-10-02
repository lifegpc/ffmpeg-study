find_package(PkgConfig)
if (PkgConfig_FOUND)
    pkg_check_modules(PC_AVUTIL QUIET libavutil)
endif()

if (PC_AVUTIL_FOUND)
    set(AVUTIL_FOUND TRUE)
    set(AVUTIL_VERSION ${PC_AVUTIL_VERSION})
    set(AVUTIL_VERSION_STRING ${PC_AVUTIL_STRING})
    if (USE_STATIC_LIBS)
        set(AVUTIL_LIBRARYS ${PC_AVUTIL_STATIC_LNIK_LIBRARIES})
        set(AVUTIL_INCLUDE_DIRS ${PC_AVUTIL_STATIC_INCLUDE_DIRS})
    else()
        set(AVUTIL_LIBRARYS ${PC_AVUTIL_LINK_LIBRARIES})
        set(AVUTIL_INCLUDE_DIRS ${PC_AVUTIL_INCLUDE_DIRS})
    endif()
    if (NOT TARGET AVUTIL::AVUTIL)
        add_library(AVUTIL::AVUTIL UNKNOWN IMPORTED)
        set_target_properties(AVUTIL::AVUTIL PROPERTIES
            IMPORTED_LOCATION "${AVUTIL_LIBRARYS}"
            INTERFACE_COMPILE_OPTIONS "${PC_AVUTIL_CFLAGS}"
            INTERFACE_INCLUDE_DIRECTORIES "${AVUTIL_INCLUDE_DIRS}"
        )
    endif()
else()
    message(FATAL_ERROR "failed.")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AVUTIL
    FOUND_VAR AVUTIL_FOUND
    REQUIRED_VARS
        AVUTIL_LIBRARYS
        AVUTIL_INCLUDE_DIRS
    VERSION_VAR AVUTIL_VERSION
)
