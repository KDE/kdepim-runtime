

if(BUILD_TESTING)
    add_subdirectory( autotests )
endif()

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_vcard_resource\")

########### next target ###############

set( vcardresource_SRCS
  vcardresource.cpp
)

install( FILES vcardresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )

kconfig_add_kcfg_files(vcardresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/vcardresource.kcfg org.kde.Akonadi.VCard.Settings)
qt5_add_dbus_adaptor(vcardresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.VCard.Settings.xml settings.h Akonadi_VCard_Resource::Settings vcardsettingsadaptor VCardSettingsAdaptor
)

add_executable(akonadi_vcard_resource ${vcardresource_SRCS})

add_subdirectory( wizard )

if( APPLE )
  set_target_properties(akonadi_vcard_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_vcard_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.VCard")
  set_target_properties(akonadi_vcard_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi VCard Resource")
endif ()

target_link_libraries(akonadi_vcard_resource KF5::AkonadiCore KF5::KIOCore   KF5::AkonadiAgentBase KF5::Contacts KF5::DBusAddons akonadi-singlefileresource)

install(TARGETS akonadi_vcard_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

