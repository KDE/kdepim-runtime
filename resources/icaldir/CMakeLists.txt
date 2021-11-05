
add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_icaldir_resource\")


set(contactsresource_common_SRCS)

kconfig_add_kcfg_files(icaldirresource_common_SRCS
    settings.kcfgc
  )
########### next target ###############

set( icaldirresource_SRCS
  icaldirresource.cpp
  icaldirresource.h
  ${icaldirresource_common_SRCS}
)

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/icaldirresource.kcfg org.kde.Akonadi.ICalDirectory.Settings)
qt_add_dbus_adaptor(icaldirresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.ICalDirectory.Settings.xml settings.h IcalDirResourceSettings
)

install( FILES icaldirresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )

add_executable(akonadi_icaldir_resource ${icaldirresource_SRCS})
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_icaldir_resource PROPERTIES UNITY_BUILD ON)
endif()

if( APPLE )
  set_target_properties(akonadi_icaldir_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_icaldir_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.ICalDirectory")
  set_target_properties(akonadi_icaldir_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi ICalDirectory Resource")
endif ()


target_link_libraries(akonadi_icaldir_resource
  KF5::AkonadiCore
  KF5::AkonadiAgentBase
  KF5::CalendarCore
  KF5::I18n
  KF5::TextWidgets
  KF5::KIOWidgets
  KF5::ConfigWidgets
)

install(TARGETS akonadi_icaldir_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

############################## Config plugin ################################
kcoreaddons_add_plugin(icaldirconfig
    INSTALL_NAMESPACE "akonadi/config"
    )
ki18n_wrap_ui(icaldirconfig icaldirsagentsettingswidget.ui)

target_sources(icaldirconfig PRIVATE
    icaldirsettingswidget.cpp
    icaldirsettingswidget.h
    ${icaldirresource_common_SRCS}
    )


target_link_libraries(icaldirconfig
    KF5::AkonadiCore
    KF5::CalendarCore
    KF5::AkonadiAgentBase
    KF5::Contacts
    KF5::AkonadiWidgets
    KF5::I18n
    KF5::KIOWidgets
    KF5::TextWidgets
    )
