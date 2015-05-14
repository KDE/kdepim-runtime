include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${Boost_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/shared
)


add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_ical_resource\")


add_subdirectory( wizard )
add_subdirectory( notes )
add_subdirectory( autotests )

########### next target ###############
add_definitions( -DSETTINGS_NAMESPACE=Akonadi_ICal_Resource )

set( icalresource_SRCS
  icalresource.cpp
  shared/icalresourcebase.cpp
  shared/icalresource.cpp
)

install( FILES icalresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )

kconfig_add_kcfg_files(icalresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/icalresource.kcfg org.kde.Akonadi.ICal.Settings)
qt5_add_dbus_adaptor(icalresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.ICal.Settings.xml settings.h Akonadi_ICal_Resource::Settings icalsettingsadaptor ICalSettingsAdaptor
)

add_executable(akonadi_ical_resource ${icalresource_SRCS})

if( APPLE )
  set_target_properties(akonadi_ical_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_ical_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.ICal")
  set_target_properties(akonadi_ical_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi ICal Resource")
endif ()

target_link_libraries(akonadi_ical_resource KF5::AkonadiCore   KF5::KIOCore KF5::CalendarCore KF5::AkonadiAgentBase akonadi-singlefileresource KF5::DBusAddons)

install(TARGETS akonadi_ical_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
