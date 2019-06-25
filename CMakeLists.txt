project(user-manager)
cmake_minimum_required(VERSION 2.8.12)
set(PROJECT_VERSION "5.16.2")

set(QT_MIN_VERSION "5.12.0")
set(KF5_MIN_VERSION "5.58.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED  COMPONENTS Core Widgets DBus)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED WidgetsAddons CoreAddons I18n Config ConfigWidgets KCMUtils KIO IconThemes Auth)
find_package(PWQuality REQUIRED)

include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)

include_directories(${PWQUALITY_INCLUDE_DIR})

# Set KI18n translation domain
add_definitions(-DTRANSLATION_DOMAIN=\"user_manager\")
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)

add_subdirectory(src)

install( FILES user-manager.categories DESTINATION ${KDE_INSTALL_CONFDIR} )

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
