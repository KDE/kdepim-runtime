add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_facebook_resource\")
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

add_library(facebookresourcelib STATIC)
ecm_qt_declare_logging_category(facebookresourcelib HEADER resource_debug.h IDENTIFIER FBRESOURCE_LOG CATEGORY_NAME org.kde.pim.fbresource)

set(fbresource_common_SRCS)
kconfig_add_kcfg_files(fbresource_common_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(settings.kcfg org.kde.Akonadi.Facebook.Settings )

target_sources(facebookresourcelib PRIVATE
    listjob.cpp
    eventslistjob.cpp eventslistjob.h
    birthdaylistjob.cpp birthdaylistjob.h
    tokenjobs.cpp tokenjobs.h
    graph.cpp graph.h
    ${fbresource_common_SRCS}
)


target_link_libraries(facebookresourcelib
    KF${KF_MAJOR_VERSION}::KIOWidgets
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::ConfigGui
    KF${KF_MAJOR_VERSION}::CalendarCore
    Qt::WebEngineWidgets
    KF5::Wallet
    KF${KF_MAJOR_VERSION}::Codecs
)

#####

add_executable(akonadi_facebook_resource resource.cpp)
target_link_libraries(akonadi_facebook_resource
    facebookresourcelib
    KPim${KF_MAJOR_VERSION}::AkonadiAgentBase
    KF${KF_MAJOR_VERSION}::CalendarCore
    KF${KF_MAJOR_VERSION}::I18n
    KF5::Wallet
    KF${KF_MAJOR_VERSION}::Codecs
    Qt::WebEngineWidgets
    KF${KF_MAJOR_VERSION}::KIOCore
)

install(TARGETS akonadi_facebook_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(
  FILES facebookresource.desktop
  DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents"
)

############################### Config plugin ################################
set(facebookconfig_ui_SRCS)
ki18n_wrap_ui(facebookconfig_ui_SRCS facebookagentsettingswidget.ui)

set(facebookconfig_SRCS
    facebooksettingswidget.cpp facebooksettingswidget.h
    ${facebookconfig_ui_SRCS}
    ${fbresource_common_SRCS}
    )


kcoreaddons_add_plugin(facebookconfig
    SOURCES ${facebookconfig_SRCS}
    INSTALL_NAMESPACE "pim${QT_MAJOR_VERSION}/akonadi/config"
    )
target_link_libraries(facebookconfig
    facebookresourcelib
    KPim${KF_MAJOR_VERSION}::AkonadiCore
    KF${KF_MAJOR_VERSION}::CalendarCore
    KF${KF_MAJOR_VERSION}::Codecs
    KPim${KF_MAJOR_VERSION}::AkonadiAgentBase
    KF${KF_MAJOR_VERSION}::Contacts
    KPim${KF_MAJOR_VERSION}::AkonadiWidgets
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::KIOWidgets
    )

