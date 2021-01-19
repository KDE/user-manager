set(user_manager_SRCS
   lib/accountmodel.cpp
   lib/modeltest.cpp
   lib/usersessions.cpp
   usermanager.cpp
   accountinfo.cpp
   createavatarjob.cpp
   passworddialog.cpp
   avatargallery.cpp
)

set_source_files_properties(lib/org.freedesktop.Accounts.xml
                        PROPERTIES NO_NAMESPACE TRUE)

set_source_files_properties(lib/org.freedesktop.Accounts.User.xml
                        PROPERTIES NO_NAMESPACE TRUE)

qt5_add_dbus_interface(user_manager_SRCS
    lib/org.freedesktop.Accounts.xml
    accounts_interface
)

qt5_add_dbus_interface(user_manager_SRCS
    lib/org.freedesktop.Accounts.User.xml
    user_interface
)

set(login1_manager_xml lib/org.freedesktop.login1.Manager.xml)
set_source_files_properties(${login1_manager_xml} PROPERTIES INCLUDE "lib/usersessions.h")
qt5_add_dbus_interface(user_manager_SRCS
    ${login1_manager_xml}
    login1_interface
)

ki18n_wrap_ui(user_manager_SRCS kcm.ui account.ui password.ui avatargallery.ui)

ecm_qt_declare_logging_category(user_manager_SRCS HEADER user_manager_debug.h IDENTIFIER USER_MANAGER_LOG CATEGORY_NAME log_user_manager DESCRIPTION "user-manager" EXPORT USERMANAGER)

add_library(user_manager MODULE ${user_manager_SRCS})

target_link_libraries(user_manager
    Qt5::Core
    Qt5::Widgets
    Qt5::DBus
    KF5::AuthCore
    KF5::WidgetsAddons
    KF5::CoreAddons
    KF5::I18n
    KF5::ConfigCore
    KF5::ConfigWidgets
    KF5::KCMUtils
    KF5::KIOCore
    ${PWQUALITY_LIBRARY}
)

if (NOT APPLE)
target_link_libraries(user_manager crypt)
endif()

install(TARGETS user_manager DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES user_manager.desktop DESTINATION ${SERVICES_INSTALL_DIR})

install(DIRECTORY pics/ DESTINATION ${DATA_INSTALL_DIR}/user-manager/avatars)
