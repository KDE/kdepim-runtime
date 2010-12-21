include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )
add_subdirectory( tests )

########### next target ###############

set( vcardresource_SRCS
  ${AKONADI_SINGLEFILERESOURCE_SHARED_SOURCES}
  vcardresource.cpp
)

install( FILES vcardresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_ui_files(vcardresource_SRCS ${AKONADI_SINGLEFILERESOURCE_SHARED_UI})
kde4_add_kcfg_files(vcardresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/vcardresource.kcfg org.kde.Akonadi.VCard.Settings)
qt4_add_dbus_adaptor(vcardresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.VCard.Settings.xml settings.h Akonadi_VCard_Resource::Settings vcardsettingsadaptor VCardSettingsAdaptor
)

if (NOT WINCE)
kde4_add_plugin(akonadi_vcard_resource ${vcardresource_SRCS})
else(NOT WINCE)
kde4_add_plugin(akonadi_vcard_resource STATIC ${vcardresource_SRCS})
endif (NOT WINCE)

if (Q_WS_MAC)
  set_target_properties(akonadi_vcard_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_vcard_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.VCard")
  set_target_properties(akonadi_vcard_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi VCard Resource")
endif (Q_WS_MAC)

target_link_libraries(akonadi_vcard_resource ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDEPIMLIBS_KABC_LIBS} ${KDE4_KIO_LIBS})

install(TARGETS akonadi_vcard_resource DESTINATION ${PLUGIN_INSTALL_DIR})

