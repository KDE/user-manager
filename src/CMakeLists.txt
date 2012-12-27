set(user_manager_SRCS
   usermanager.cpp
)

set_source_files_properties(lib/org.freedesktop.Accounts.xml
                        PROPERTIES NO_NAMESPACE TRUE)

set_source_files_properties(lib/org.freedesktop.Accounts.User.xml
                        PROPERTIES NO_NAMESPACE TRUE)

qt4_add_dbus_interface(user_manager_SRCS
    lib/org.freedesktop.Accounts.xml
    accounts_interface
)

qt4_add_dbus_interface(user_manager_SRCS
    lib/org.freedesktop.Accounts.User.xml
    user_interface
)

kde4_add_ui_files(user_manager_SRCS kcm.ui)

kde4_add_plugin(user_manager ${user_manager_SRCS})

target_link_libraries(user_manager ${KDE4_KDEUI_LIBS})

install(TARGETS user_manager DESTINATION ${PLUGIN_INSTALL_DIR})

install(PROGRAMS user_manager.desktop DESTINATION ${SERVICES_INSTALL_DIR})