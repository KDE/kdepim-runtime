include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ical/shared
)



########### next target ###############
add_definitions(-DSETTINGS_NAMESPACE=Akonadi_KAlarm_Resource)

set(kalarmresource_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ical/shared/icalresourcebase.cpp
    kalarmresource.cpp
    ../shared/kalarmresourcecommon.cpp
    ../shared/alarmtyperadiowidget.cpp
)

install(FILES kalarmresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")

ki18n_wrap_ui(kalarmresource_SRCS ../shared/alarmtyperadiowidget.ui)
kconfig_add_kcfg_files(kalarmresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/kalarmresource.kcfg org.kde.Akonadi.KAlarm.Settings)
qt5_add_dbus_adaptor(kalarmresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.KAlarm.Settings.xml settings.h Akonadi_KAlarm_Resource::Settings icalsettingsadaptor ICalSettingsAdaptor)

ecm_qt_declare_logging_category(kalarmresource_SRCS HEADER kalarmresource_debug.h IDENTIFIER KALARMRESOURCE_LOG CATEGORY_NAME org.kde.pim.kalarmresource)

add_custom_target(kalarm_resource_xml ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.KAlarm.Settings.xml)

add_executable(akonadi_kalarm_resource ${kalarmresource_SRCS})

if( APPLE )
    set_target_properties(akonadi_kalarm_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
    set_target_properties(akonadi_kalarm_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.KAlarm")
    set_target_properties(akonadi_kalarm_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi KAlarm Resource")
endif ()

target_link_libraries(akonadi_kalarm_resource
                      KF5::AlarmCalendar
                      KF5::AkonadiCore
                      KF5::CalendarCore
                      KF5::KIOCore
                      KF5::AkonadiAgentBase
                      KF5::DBusAddons
                      akonadi-singlefileresource
                     )

install(TARGETS akonadi_kalarm_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
