include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/libmaildir
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

add_subdirectory( tests )
add_subdirectory( wizard )

# maildir access library
add_subdirectory(libmaildir)


########### next target ###############

set( maildirresource_SRCS
  maildirresource.cpp
  configdialog.cpp
  main.cpp
)

install( FILES maildirresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_ui_files(maildirresource_SRCS settings.ui)

kde4_add_kcfg_files(maildirresource_SRCS settings.kcfgc)

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/maildirresource.kcfg org.kde.Akonadi.Maildir.Settings)

qt4_add_dbus_adaptor(maildirresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Maildir.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_maildir_resource ${maildirresource_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_maildir_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_maildir_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.Maildir")
  set_target_properties(akonadi_maildir_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Maildir Resource")
endif (Q_WS_MAC)

target_link_libraries(akonadi_maildir_resource ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} maildir ${QT_QTDBUS_LIBRARY} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KMIME_LIBS})

install(TARGETS akonadi_maildir_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Maildir.Settings.xml
        DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})
