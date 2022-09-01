set(RSSBOTLIB_VERSION_MAJOR 1)
set(RSSBOTLIB_VERSION_MINOR 1)
set(RSSBOTLIB_VERSION_MICRO 0)
set(RSSBOTLIB_VERSION_REV 0)
set(RSSBOTLIB_VERSION ${RSSBOTLIB_VERSION_MAJOR}.${RSSBOTLIB_VERSION_MINOR}.${RSSBOTLIB_VERSION_MICRO}.${RSSBOTLIB_VERSION_REV})
if (WIN32)
    message(STATUS "Generate \"${CMAKE_CURRENT_BINARY_DIR}/rssbotlib.rc\"")
    configure_file("${CMAKE_CURRENT_LIST_DIR}/rssbotlib.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/rssbotlib.rc")
endif()
