include_directories(
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ical/shared
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

########### next target ###############
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=5952)
add_definitions(-DSETTINGS_NAMESPACE=Akonadi_KAlarm_Resource)

set(kalarmresource_SRCS
    ${AKONADI_SINGLEFILERESOURCE_SHARED_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ical/shared/icalresourcebase.cpp
    kalarmresource.cpp
    ../shared/kalarmresourcecommon.cpp
    ../shared/alarmtyperadiowidget.cpp
)

install(FILES kalarmresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")

kde4_add_ui_files(kalarmresource_SRCS ../shared/alarmtyperadiowidget.ui ${AKONADI_SINGLEFILERESOURCE_SHARED_UI})
kde4_add_kcfg_files(kalarmresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/kalarmresource.kcfg org.kde.Akonadi.KAlarm.Settings)
qt4_add_dbus_adaptor(kalarmresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.KAlarm.Settings.xml settings.h Akonadi_KAlarm_Resource::Settings icalsettingsadaptor ICalSettingsAdaptor)
add_custom_target(kalarm_resource_xml ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.KAlarm.Settings.xml)

if (NOT WINCE)
    kde4_add_plugin(akonadi_kalarm_resource ${kalarmresource_SRCS})
else(NOT WINCE)
    kde4_add_plugin(akonadi_kalarm_resource STATIC ${kalarmresource_SRCS})
endif(NOT WINCE)

if (Q_WS_MAC)
    set_target_properties(akonadi_kalarm_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
    set_target_properties(akonadi_kalarm_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.KAlarm")
    set_target_properties(akonadi_kalarm_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi KAlarm Resource")
endif (Q_WS_MAC)

target_link_libraries(akonadi_kalarm_resource
                      ${KDEPIMLIBS_KALARMCAL_LIBS}
                      ${KDEPIMLIBS_AKONADI_KCAL_LIBS}
                      ${KDEPIMLIBS_AKONADI_LIBS}
                      ${KDEPIMLIBS_KCALCORE_LIBS}
                      ${KDE4_KIO_LIBS}
                      ${QT_QTDBUS_LIBRARY}
                     )

install(TARGETS akonadi_kalarm_resource DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.KAlarm.Settings.xml
        DESTINATION ${AKONADI_DBUS_INTERFACES_DIR})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/kalarmresource.kcfg
        DESTINATION ${KCFG_INSTALL_DIR})
