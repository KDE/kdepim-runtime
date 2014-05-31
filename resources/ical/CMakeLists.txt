include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/shared
)

kde_enable_exceptions()

add_subdirectory( wizard )
add_subdirectory( notes )
#REACTIVATE IT
#add_subdirectory( tests )

########### next target ###############
add_definitions( -DSETTINGS_NAMESPACE=Akonadi_ICal_Resource )

set( icalresource_SRCS
  ${AKONADI_SINGLEFILERESOURCE_SHARED_SOURCES}
  shared/icalresourcebase.cpp
  shared/icalresource.cpp
  icalresourceplugin.cpp
)

install( FILES icalresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

ki18n_wrap_ui(icalresource_SRCS ${AKONADI_SINGLEFILERESOURCE_SHARED_UI})
kconfig_add_kcfg_files(icalresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/icalresource.kcfg org.kde.Akonadi.ICal.Settings)
qt5_add_dbus_adaptor(icalresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.ICal.Settings.xml settings.h Akonadi_ICal_Resource::Settings icalsettingsadaptor ICalSettingsAdaptor
)

add_library(akonadi_ical_resource MODULE ${icalresource_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_ical_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_ical_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.ICal")
  set_target_properties(akonadi_ical_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi ICal Resource")
endif ()

target_link_libraries(akonadi_ical_resource KF5::AkonadiCore   KF5::KIOCore KF5::CalendarCore KF5::AkonadiAgentBase)

install(TARGETS akonadi_ical_resource DESTINATION ${PLUGIN_INSTALL_DIR})
