include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

add_subdirectory( wizard )
if (NOT WINCE)
add_subdirectory( notes )
add_subdirectory( tests )
endif(NOT WINCE)

########### next target ###############
add_definitions( -DSETTINGS_NAMESPACE=Akonadi_ICal_Resource )

set( icalresource_SRCS
  ${AKONADI_SINGLEFILERESOURCE_SHARED_SOURCES}
  icalresourcebase.cpp
  icalresource.cpp
  icalresourceplugin.cpp
)

install( FILES icalresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_ui_files(icalresource_SRCS ${AKONADI_SINGLEFILERESOURCE_SHARED_UI})
kde4_add_kcfg_files(icalresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/icalresource.kcfg org.kde.Akonadi.ICal.Settings)
qt4_add_dbus_adaptor(icalresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.ICal.Settings.xml settings.h Akonadi_ICal_Resource::Settings icalsettingsadaptor ICalSettingsAdaptor
)

if (NOT WINCE)
kde4_add_plugin(akonadi_ical_resource ${icalresource_SRCS})
else(NOT WINCE)
kde4_add_plugin(akonadi_ical_resource STATIC ${icalresource_SRCS})
endif(NOT WINCE)

if (Q_WS_MAC)
  set_target_properties(akonadi_ical_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_ical_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.ICal")
  set_target_properties(akonadi_ical_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi ICal Resource")
endif (Q_WS_MAC)

target_link_libraries(akonadi_ical_resource ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTDBUS_LIBRARY}  ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KCALCORE_LIBS})

install(TARGETS akonadi_ical_resource DESTINATION ${PLUGIN_INSTALL_DIR})
