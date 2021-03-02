include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/shared
)

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_ical_resource\")

kpackage_install_package(wizard org.kde.pim.account.ical packages wizard)

add_subdirectory( notes )

if(BUILD_TESTING)
    add_subdirectory( autotests )
endif()

set(icalresource_common_SRCS)
kconfig_add_kcfg_files(icalresource_common_SRCS settings.kcfgc)

############################# Resource #################################
add_definitions( -DSETTINGS_NAMESPACE=Akonadi_ICal_Resource )

set( icalresource_SRCS
  icalresource.cpp
  shared/icalresourcebase.cpp
  shared/icalresource.cpp
  ${icalresource_common_SRCS}
)

install( FILES icalresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/icalresource.kcfg org.kde.Akonadi.ICal.Settings)
qt5_add_dbus_adaptor(icalresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.ICal.Settings.xml settings.h Akonadi_ICal_Resource::Settings icalsettingsadaptor ICalSettingsAdaptor
)

add_executable(akonadi_ical_resource ${icalresource_SRCS})
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_ical_resource PROPERTIES UNITY_BUILD ON)
endif()

if( APPLE )
  set_target_properties(akonadi_ical_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_ical_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.ICal")
  set_target_properties(akonadi_ical_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi iCal Resource")
endif ()

target_link_libraries(akonadi_ical_resource KF5::AkonadiCore   KF5::KIOCore KF5::CalendarCore KF5::AkonadiAgentBase akonadi-singlefileresource)
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_ical_resource PROPERTIES UNITY_BUILD ON)
endif()
install(TARGETS akonadi_ical_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

################################ Config plugin ###############################

kcoreaddons_add_plugin(icalconfig
    SOURCES icalconfig.cpp ${icalresource_common_SRCS}
    JSON "icalconfig.json"
    INSTALL_NAMESPACE "akonadi/config"
)
target_link_libraries(icalconfig
    KF5::AkonadiCore
    akonadi-singlefileresource
)
