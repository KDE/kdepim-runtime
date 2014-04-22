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
  endmacro()
endif ()

include_directories(
    ${KDE4_INCLUDES}
    ${KDEPIMLIBS_INCLUDE_DIRS}
)

kde_enable_exceptions()


########### next target ###############

set( pop3resource_SRCS
  pop3resource.cpp
  accountdialog.cpp
  jobs.cpp
  settings.cpp
  pop3resourceattribute.cpp
)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=5266)

install( FILES pop3resource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

qt5_wrap_ui( pop3resource_SRCS popsettings.ui)
kconfig_add_kcfg_files(pop3resource_SRCS settingsbase.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/settings.kcfg org.kde.Akonadi.POP3.Settings)
qt5_add_dbus_adaptor(pop3resource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.POP3.Settings.xml settings.h Settings
)

add_executable(akonadi_pop3_resource RUN_UNINSTALLED ${pop3resource_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_pop3_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_pop3_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.POP3")
  set_target_properties(akonadi_pop3_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi POP3 Resource")
endif ()


target_link_libraries( akonadi_pop3_resource
  KF5::AkonadiCore
  KF5::AkonadiKMime
  
  
  
  ${KDE4_KDECORE_LIBS}
  KF5::KIOCore
  KF5::KMime
  KF5::MailTransport
  KF5::PimUtils
)

add_subdirectory( tests )
add_subdirectory( wizard )

install(TARGETS akonadi_pop3_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
