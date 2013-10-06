set(user_manager_SRCS
   lib/accountmodel.cpp
   lib/modeltest.cpp
   lib/usersessions.cpp
   usermanager.cpp
   accountinfo.cpp
   createavatarjob.cpp
   passworddialog.cpp
   passwordedit.cpp
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

set(login1_manager_xml lib/org.freedesktop.login1.Manager.xml)
set_source_files_properties(${login1_manager_xml} PROPERTIES INCLUDE "lib/usersessions.h")
qt4_add_dbus_interface(user_manager_SRCS
    ${login1_manager_xml}
    login1_interface
)


kde4_add_ui_files(user_manager_SRCS kcm.ui account.ui password.ui)

kde4_add_plugin(user_manager ${user_manager_SRCS})

target_link_libraries(user_manager ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${PWQUALITY_LIBRARY} crypt)

install(TARGETS user_manager DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES user_manager.desktop DESTINATION ${SERVICES_INSTALL_DIR})
