project(user-manager)
cmake_minimum_required(VERSION 2.8.12)
set(PROJECT_VERSION "5.13.90")

set(QT_MIN_VERSION "5.11.0")
set(KF5_MIN_VERSION "5.50.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED  COMPONENTS Core Widgets DBus)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED WidgetsAddons CoreAddons I18n Config ConfigWidgets KCMUtils KIO IconThemes Auth)
find_package(PWQuality REQUIRED)

include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

include_directories(${PWQUALITY_INCLUDE_DIR})

# Set KI18n translation domain
add_definitions(-DTRANSLATION_DOMAIN=\"user_manager\")
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)
add_definitions(-DQT_NO_CAST_FROM_ASCII)
add_definitions(-DQT_NO_CAST_TO_ASCII)
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)
add_definitions(-DQT_USE_QSTRINGBUILDER)

add_subdirectory(src)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
