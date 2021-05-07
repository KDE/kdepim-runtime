include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared
    )

########### next target ###############
add_definitions(-DSETTINGS_NAMESPACE=Akonadi_KAlarm_Dir_Resource)

set(kalarmdirresource_SRCS
    kalarmdirresource_debug.cpp
    settingsdialog.cpp
    kalarmdirresource.cpp
    ../shared/kalarmresourcecommon.cpp
    ../shared/alarmtypewidget.cpp
    )

install(FILES kalarmdirresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")

ki18n_wrap_ui(kalarmdirresource_SRCS settingsdialog.ui ../shared/alarmtypewidget.ui)
kconfig_add_kcfg_files(kalarmdirresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/kalarmdirresource.kcfg org.kde.Akonadi.KAlarmDir.Settings)
qt_add_dbus_adaptor(kalarmdirresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.KAlarmDir.Settings.xml settings.h Akonadi_KAlarm_Dir_Resource::Settings kalarmdirsettingsadaptor KAlarmDirSettingsAdaptor)

ecm_qt_declare_logging_category(kalarmdirresource_SRCS HEADER kalarmdirresource_debug.h IDENTIFIER KALARMDIRRESOURCE_LOG CATEGORY_NAME org.kde.pim.kalarmdirresource
    DESCRIPTION "kalarm directory resource (kdepim-runtime)"
    OLD_CATEGORY_NAMES log_kalarmdirresource
    EXPORT KDEPIMRUNTIME
    )


add_custom_target(kalarmdir_resource_xml ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.KAlarmDir.Settings.xml)

add_executable(akonadi_kalarm_dir_resource ${kalarmdirresource_SRCS})
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_kalarm_dir_resource PROPERTIES UNITY_BUILD ON)
endif()
if( APPLE )
    set_target_properties(akonadi_kalarm_dir_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
    set_target_properties(akonadi_kalarm_dir_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.KAlarmDir")
    set_target_properties(akonadi_kalarm_dir_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi KAlarm Directory Resource")
endif ()

target_link_libraries(akonadi_kalarm_dir_resource
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::ConfigWidgets
    KF5::WindowSystem
    KF5::AlarmCalendar
    KF5::AkonadiCore
    KF5::CalendarCore
    KF5::AkonadiAgentBase
    )

install(TARGETS akonadi_kalarm_dir_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
