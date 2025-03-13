add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_vcarddir_resource\")

########### next target ###############

set(vcarddirresource_common_SRCS)

kconfig_add_kcfg_files(vcarddirresource_common_SRCS
    settings.kcfgc
)

set(vcarddirresource_SRCS
    ${vcarddirresource_common_SRCS}
  vcarddirresource.cpp
  vcarddirresource.h
)
ecm_qt_declare_logging_category(vcarddirresource_SRCS HEADER vcarddirresource_debug.h IDENTIFIER VCARDDIRRESOURCE_LOG CATEGORY_NAME org.kde.pim.vcarddirresource
        DESCRIPTION "vcarddir resource (kdepim-runtime)"
        EXPORT KDEPIMRUNTIME
)



kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/vcarddirresource.kcfg org.kde.Akonadi.VCardDirectory.Settings)
qt_add_dbus_adaptor(vcarddirresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.VCardDirectory.Settings.xml settings.h VcardDirResourceSettings
)

install(FILES vcarddirresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")

add_executable(akonadi_vcarddir_resource ${vcarddirresource_SRCS})

if(APPLE)
  set_target_properties(akonadi_vcarddir_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_vcarddir_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.VCardDirectory")
  set_target_properties(akonadi_vcarddir_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi VCardDirectory Resource")
endif()


target_link_libraries(akonadi_vcarddir_resource
  KPim6::AkonadiCore
  KPim6::AkonadiAgentBase
  KF6::Contacts
  KF6::I18n
  KF6::WindowSystem
  Qt::DBus
)

add_dependencies(akonadi_vcarddir_resource akonadi-singlefileresource)

install(TARGETS akonadi_vcarddir_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(
    FILES org.kde.akonadi_vcarddir_resource.desktop
    DESTINATION ${KDE_INSTALL_APPDIR}
)

############################## Config plugin ################################
kcoreaddons_add_plugin(vcarddirconfig
    INSTALL_NAMESPACE "pim6/akonadi/config"
)
ki18n_wrap_ui(vcarddirconfig vcarddiragentsettingswidget.ui)

target_sources(vcarddirconfig PRIVATE
    vcarddirsettingswidget.cpp
    vcarddirsettingswidget.h
    ${vcarddirconfig_ui_SRCS}
    ${vcarddirresource_common_SRCS}
)

target_link_libraries(vcarddirconfig
    KPim6::AkonadiCore
    KF6::CalendarCore
    KPim6::AkonadiAgentBase
    KPim6::AkonadiWidgets
    KF6::I18n
    KF6::KIOWidgets
)
if(HAVE_ACTIVITY_SUPPORT)
     target_link_libraries(vcarddirconfig
         KPim6::PimCommonActivities
     )
endif()
