add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_vcarddir_resource\")

########### next target ###############

set(vcarddirresource_common_SRCS)

kconfig_add_kcfg_files(vcarddirresource_common_SRCS
    settings.kcfgc
  )

set( vcarddirresource_SRCS
    ${vcarddirresource_common_SRCS}
  vcarddirresource.cpp
  dirsettingsdialog.cpp
)
ecm_qt_declare_logging_category(vcarddirresource_SRCS HEADER vcarddirresource_debug.h IDENTIFIER VCARDDIRRESOURCE_LOG CATEGORY_NAME org.kde.pim.vcarddirresource)


kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/vcarddirresource.kcfg org.kde.Akonadi.VCardDirectory.Settings)
qt5_add_dbus_adaptor(vcarddirresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.VCardDirectory.Settings.xml settings.h Settings
)

install( FILES vcarddirresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )

add_executable(akonadi_vcarddir_resource ${vcarddirresource_SRCS})

if( APPLE )
  set_target_properties(akonadi_vcarddir_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_vcarddir_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.VCardDirectory")
  set_target_properties(akonadi_vcarddir_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi VCardDirectory Resource")
endif ()


target_link_libraries(akonadi_vcarddir_resource
  KF5::AkonadiCore
  KF5::AkonadiAgentBase
  KF5::Contacts
  KF5::I18n
  KF5::TextWidgets
  KF5::KIOWidgets
  KF5::ConfigWidgets
  KF5::WindowSystem
  Qt5::DBus
)

add_dependencies(akonadi_vcarddir_resource akonadi-singlefileresource)

add_subdirectory(wizard)

install(TARGETS akonadi_vcarddir_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

############################## Config plugin ################################
#set(contactsconfig_ui_SRCS)
#ki18n_wrap_ui(contactsconfig_ui_SRCS contactsagentsettingswidget.ui)

#set(contactsconfig_SRCS
#    contactssettingswidget.cpp
#    ${contactsconfig_ui_SRCS}
#    ${vcarddirresource_common_SRCS}
#    )


#kcoreaddons_add_plugin(contactsconfig
#    SOURCES ${contactsconfig_SRCS}
#    JSON "contactsconfig.json"
#    INSTALL_NAMESPACE "akonadi/config"
#    )
#target_link_libraries(contactsconfig
#    KF5::AkonadiCore
#    KF5::CalendarCore
#    KF5::Codecs
#    KF5::AkonadiAgentBase
#    KF5::Contacts
#    KF5::AkonadiWidgets
#    KF5::I18n
#    KF5::KIOWidgets
#    )
