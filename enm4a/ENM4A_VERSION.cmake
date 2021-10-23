set(ENM4A_VERSION_MAJOR 1)
set(ENM4A_VERSION_MINOR 0)
set(ENM4A_VERSION_MICRO 1)
set(ENM4A_VERSION_REV 4)
set(ENM4A_VERSION ${ENM4A_VERSION_MAJOR}.${ENM4A_VERSION_MINOR}.${ENM4A_VERSION_MICRO}.${ENM4A_VERSION_REV})
message(STATUS "Generate \"${CMAKE_CURRENT_BINARY_DIR}/enm4a_version.h\"")
configure_file("${CMAKE_CURRENT_LIST_DIR}/enm4a_version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/enm4a_version.h")
if (WIN32)
    message(STATUS "Generate \"${CMAKE_CURRENT_BINARY_DIR}/enm4a.rc\"")
    configure_file("${CMAKE_CURRENT_LIST_DIR}/enm4a.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/enm4a.rc")
endif()
