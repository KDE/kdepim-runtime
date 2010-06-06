project(pop3)

if (XSLTPROC_EXECUTABLE)
  # generates a D-Bus interface description from a KConfigXT file
  macro( kcfg_generate_dbus_interface _kcfg _name )
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
      COMMAND ${XSLTPROC_EXECUTABLE} --stringparam interfaceName ${_name}
              ${KDEPIMLIBS_DATA_DIR}/akonadi-kde/kcfg2dbus.xsl
              ${_kcfg}
              > ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
      DEPENDS ${KDEPIMLIBS_DATA_DIR}/akonadi-kde/kcfg2dbus.xsl
              ${_kcfg}
    )
  endmacro( kcfg_generate_dbus_interface )
endif (XSLTPROC_EXECUTABLE)

include_directories(
    ${KDE4_INCLUDES}
    ${KDEPIMLIBS_INCLUDE_DIRS}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( pop3resource_SRCS
  pop3resource.cpp
  accountdialog.cpp
  jobs.cpp
  settings.cpp
)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=5266)

install( FILES pop3resource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_ui_files( pop3resource_SRCS popsettings.ui)
kde4_add_kcfg_files(pop3resource_SRCS settingsbase.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/settings.kcfg org.kde.Akonadi.POP3.Settings)
qt4_add_dbus_adaptor(pop3resource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.POP3.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_pop3_resource RUN_UNINSTALLED ${pop3resource_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_pop3_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_pop3_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.POP3")
  set_target_properties(akonadi_pop3_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi POP3 Resource")
endif (Q_WS_MAC)


target_link_libraries( akonadi_pop3_resource
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KIO_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${KDE4_MAILTRANSPORT_LIBRARY}
)

add_subdirectory( tests )
add_subdirectory( wizard )

install(TARGETS akonadi_pop3_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
