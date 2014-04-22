include_directories(
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared
)

########### next target ###############
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=5953)
add_definitions(-DSETTINGS_NAMESPACE=Akonadi_KAlarm_Dir_Resource)

set(kalarmdirresource_SRCS
    settingsdialog.cpp
    kalarmdirresource.cpp
    ../shared/kalarmresourcecommon.cpp
    ../shared/alarmtypewidget.cpp
)

install(FILES kalarmdirresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")

qt5_wrap_ui(kalarmdirresource_SRCS settingsdialog.ui ../shared/alarmtypewidget.ui)
kconfig_add_kcfg_files(kalarmdirresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/kalarmdirresource.kcfg org.kde.Akonadi.KAlarmDir.Settings)
qt5_add_dbus_adaptor(kalarmdirresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.KAlarmDir.Settings.xml settings.h Akonadi_KAlarm_Dir_Resource::Settings kalarmdirsettingsadaptor KAlarmDirSettingsAdaptor)
add_custom_target(kalarmdir_resource_xml ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.KAlarmDir.Settings.xml)

add_library(akonadi_kalarm_dir_resource MODULE ${kalarmdirresource_SRCS})

if (Q_WS_MAC)
    set_target_properties(akonadi_kalarm_dir_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
    set_target_properties(akonadi_kalarm_dir_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.KAlarmDir")
    set_target_properties(akonadi_kalarm_dir_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi KAlarm Directory Resource")
endif ()

target_link_libraries(akonadi_kalarm_dir_resource
                      KF5::KAlarmCal
                      ${KDEPIMLIBS_AKONADI_KCAL_LIBS}
                      KF5::AkonadiCore
                      KF5::CalendarCore
                      KF5::KIOCore
                      KF5::AkonadiAgentBase 
                     )

install(TARGETS akonadi_kalarm_dir_resource DESTINATION ${PLUGIN_INSTALL_DIR})
