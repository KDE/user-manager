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


qt5_wrap_ui(user_manager_SRCS kcm.ui account.ui password.ui)

add_library(user_manager MODULE ${user_manager_SRCS})

target_link_libraries(user_manager
    KF5::KDELibs4Support
    ${PWQUALITY_LIBRARY}
    crypt
)

install(TARGETS user_manager DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES user_manager.desktop DESTINATION ${SERVICES_INSTALL_DIR})
