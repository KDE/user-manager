project(user-manager)
cmake_minimum_required(VERSION 2.8.12)
set(PROJECT_VERSION "5.12.9")

find_package(ECM 1.3.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

find_package(Qt5 REQUIRED COMPONENTS Core Widgets DBus)
find_package(KF5 REQUIRED COMPONENTS WidgetsAddons CoreAddons I18n Config ConfigWidgets KCMUtils KIO IconThemes Auth)
find_package(PWQuality REQUIRED)

include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

include_directories(${PWQUALITY_INCLUDE_DIR})

# Set KI18n translation domain
add_definitions(-DTRANSLATION_DOMAIN=\"user_manager\")
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

add_subdirectory(src)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
