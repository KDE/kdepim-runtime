include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../maildir
    ${CMAKE_CURRENT_BINARY_DIR}/../maildir
    ${CMAKE_CURRENT_SOURCE_DIR}/../maildir/libmaildir
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( akonotesresource_SRCS
  ../maildir/maildirresource.cpp
  ../maildir/configdialog.cpp
  akonotesresource.cpp

  main.cpp
)


kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/../maildir/maildirresource.kcfg org.kde.Akonadi.Maildir.Settings)

kde4_add_ui_files(akonotesresource_SRCS ../maildir/settings.ui)

kde4_add_kcfg_files(akonotesresource_SRCS ../maildir/settings.kcfgc)

qt4_add_dbus_adaptor(akonotesresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Maildir.Settings.xml settings.h Settings
)

install( FILES akonotesresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

if (NOT WINCE)
  kde4_add_plugin(akonadi_akonotes_resource ${akonotesresource_SRCS})
else(NOT WINCE)
  kde4_add_plugin(akonadi_akonotes_resource STATIC ${akonotesresource_SRCS})
endif(NOT WINCE)


target_link_libraries(akonadi_akonotes_resource ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} maildir ${QT_QTDBUS_LIBRARY} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KMIME_LIBS})

install(TARGETS akonadi_akonotes_resource DESTINATION ${PLUGIN_INSTALL_DIR})

