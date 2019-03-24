include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ical/shared
)

set(kalarmresource_common_SRCS)
kconfig_add_kcfg_files(kalarmresource_common_SRCS settings.kcfgc)

############################ Resource  ##################################
add_definitions(-DSETTINGS_NAMESPACE=Akonadi_KAlarm_Resource)

set(kalarmresource_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ical/shared/icalresourcebase.cpp
    kalarmresource.cpp
    ../shared/kalarmresourcecommon.cpp
    ../shared/alarmtyperadiowidget.cpp
    ${kalarmresource_common_SRCS}
)

install(FILES kalarmresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/kalarmresource.kcfg org.kde.Akonadi.KAlarm.Settings)
qt5_add_dbus_adaptor(kalarmresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.KAlarm.Settings.xml settings.h Akonadi_KAlarm_Resource::Settings icalsettingsadaptor ICalSettingsAdaptor)

ecm_qt_declare_logging_category(kalarmresource_SRCS HEADER kalarmresource_debug.h IDENTIFIER KALARMRESOURCE_LOG CATEGORY_NAME org.kde.pim.kalarmresource)

add_custom_target(kalarm_resource_xml ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.KAlarm.Settings.xml)

# ui_alarmtyperadiowidget.h is used by both akonadi_kalarm_resource and kalarmconfig
ki18n_wrap_ui(kalarmresource_shared_SRCS ../shared/alarmtyperadiowidget.ui)
add_library(kalarmresource_shared_object OBJECT ${kalarmresource_shared_SRCS})
# The asan build on FreeBSD doesn't like the empty moc object
set_target_properties(kalarmresource_shared_object PROPERTIES AUTOMOC OFF)

add_executable(akonadi_kalarm_resource
  $<TARGET_OBJECTS:kalarmresource_shared_object>
  ${kalarmresource_SRCS}
)

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
                      KF5::I18n
                      akonadi-singlefileresource
                     )

install(TARGETS akonadi_kalarm_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


############################## Config plugin #################################

set(kalarmconfig_SRCS
    $<TARGET_OBJECTS:kalarmresource_shared_object>
    kalarmconfig.cpp
    ../shared/alarmtyperadiowidget.cpp
    ${kalarmresource_common_SRCS}
)

kcoreaddons_add_plugin(kalarmconfig
    SOURCES ${kalarmconfig_SRCS}
    JSON "kalarmconfig.json"
    INSTALL_NAMESPACE "akonadi/config"
)

target_link_libraries(kalarmconfig
    KF5::AkonadiCore
    KF5::AlarmCalendar
    akonadi-singlefileresource
)
