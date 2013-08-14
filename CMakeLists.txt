project(user-manager)
SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

find_package(KDE4 REQUIRED)
find_package(PWQuality REQUIRED)

include(KDE4Defaults)

include_directories(${KDE4_INCLUDES} ${QT_INCLUDES} ${PWQUALITY_INCLUDE_DIR})

add_subdirectory(src)
