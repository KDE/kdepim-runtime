include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/libmaildir
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

add_subdirectory( tests )

# maildir access library
add_subdirectory(libmaildir)


########### next target ###############

set( maildirresource_SRCS
  maildirresource.cpp
  configdialog.cpp
)

install( FILES maildirresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_ui_files(maildirresource_SRCS settings.ui)

kde4_add_kcfg_files(maildirresource_SRCS settings.kcfgc)

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/maildirresource.kcfg org.kde.Akonadi.Maildir.Settings)

qt4_add_dbus_adaptor(maildirresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Maildir.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_maildir_resource RUN_UNINSTALLED ${maildirresource_SRCS})

target_link_libraries(akonadi_maildir_resource ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} maildir ${QT_QTDBUS_LIBRARY} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KMIME_LIBS})

install(TARGETS akonadi_maildir_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Maildir.Settings.xml
        DESTINATION ${KDE4_DBUS_INTERFACES_DIR})
