add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_facebook_resource\")
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

add_library(facebookresourcelib STATIC)
ecm_qt_declare_logging_category(facebookresourcelib HEADER resource_debug.h IDENTIFIER FBRESOURCE_LOG CATEGORY_NAME org.kde.pim.fbresource)

set(fbresource_common_SRCS)
kconfig_add_kcfg_files(fbresource_common_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(settings.kcfg org.kde.Akonadi.Facebook.Settings )

target_sources(facebookresourcelib PRIVATE
    listjob.cpp
    eventslistjob.cpp
    birthdaylistjob.cpp
    tokenjobs.cpp
    graph.cpp
    ${fbresource_common_SRCS}
)


target_link_libraries(facebookresourcelib
    KF5::KIOWidgets
    KF5::I18n
    KF5::ConfigGui
    KF5::CalendarCore
    Qt${QT_MAJOR_VERSION}::WebEngineWidgets
    KF5::Wallet
    KF5::Codecs
)

#####

add_executable(akonadi_facebook_resource resource.cpp)
target_link_libraries(akonadi_facebook_resource
    facebookresourcelib
    KF5::AkonadiAgentBase
    KF5::CalendarCore
    KF5::I18n
    KF5::Wallet
    KF5::Codecs
    Qt${QT_MAJOR_VERSION}::WebEngineWidgets
    KF5::KIOCore
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
    facebooksettingswidget.cpp
    ${facebookconfig_ui_SRCS}
    ${fbresource_common_SRCS}
    )


kcoreaddons_add_plugin(facebookconfig
    SOURCES ${facebookconfig_SRCS}
    INSTALL_NAMESPACE "pim${QT_MAJOR_VERSION}/akonadi/config"
    )
target_link_libraries(facebookconfig
    facebookresourcelib
    KF5::AkonadiCore
    KF5::CalendarCore
    KF5::Codecs
    KF5::AkonadiAgentBase
    KF5::Contacts
    KF5::AkonadiWidgets
    KF5::I18n
    KF5::KIOWidgets
    )

