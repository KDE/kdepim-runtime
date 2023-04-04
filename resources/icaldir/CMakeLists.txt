
add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_icaldir_resource\")


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
  KPim6::AkonadiCore
  KPim6::AkonadiAgentBase
  KF6::CalendarCore
  KF6::I18n
  KF6::TextWidgets
  KF6::KIOWidgets
  KF6::ConfigWidgets
)

install(TARGETS akonadi_icaldir_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

############################## Config plugin ################################
kcoreaddons_add_plugin(icaldirconfig
    INSTALL_NAMESPACE "pim${QT_MAJOR_VERSION}/akonadi/config"
    )
ki18n_wrap_ui(icaldirconfig icaldirsagentsettingswidget.ui)

target_sources(icaldirconfig PRIVATE
    icaldirsettingswidget.cpp
    icaldirsettingswidget.h
    ${icaldirresource_common_SRCS}
    )


target_link_libraries(icaldirconfig
    KPim6::AkonadiCore
    KF6::CalendarCore
    KPim6::AkonadiAgentBase
    KF6::Contacts
    KPim6::AkonadiWidgets
    KF6::I18n
    KF6::KIOWidgets
    KF6::TextWidgets
    )
