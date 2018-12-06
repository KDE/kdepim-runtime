add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_facebook_resource\")
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})
set(fbresource_common_SRCS)
kconfig_add_kcfg_files(fbresource_common_SRCS settings.kcfgc)

set(fbresource_SRCS
    resource.cpp
    listjob.cpp
    eventslistjob.cpp
    birthdaylistjob.cpp
    tokenjobs.cpp
    graph.cpp
    ${fbresource_common_SRCS}
)

ki18n_wrap_ui(fbresource_SRCS
    settingsdialog.ui
)

ecm_qt_declare_logging_category(fbresource_SRCS HEADER resource_debug.h IDENTIFIER FBRESOURCE_LOG CATEGORY_NAME org.kde.pim.fbresource)

kcfg_generate_dbus_interface(settings.kcfg org.kde.Akonadi.Facebook.Settings )

add_executable(akonadi_facebook_resource ${fbresource_SRCS})
target_link_libraries(akonadi_facebook_resource
    KF5::AkonadiAgentBase
    KF5::CalendarCore
    KF5::I18n
    KF5::Wallet
    KF5::Codecs
    Qt5::WebEngineWidgets
    KF5::KIOCore
)

install(TARGETS akonadi_facebook_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(
  FILES facebookresource.desktop
  DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents"
)

############################### Config plugin ################################
#set(facebookconfig_ui_SRCS)
#ki18n_wrap_ui(facebookconfig_ui_SRCS facebookagentsettingswidget.ui)

#set(facebookconfig_SRCS
#    facebooksettingswidget.cpp
#    ${facebookconfig_ui_SRCS}
#    ${fbresource_common_SRCS}
#    )


#kcoreaddons_add_plugin(facebookconfig
#    SOURCES ${facebookconfig_SRCS}
#    JSON "facebookconfig.json"
#    INSTALL_NAMESPACE "akonadi/config"
#    )
#target_link_libraries(facebookconfig
#    KF5::AkonadiCore
#    KF5::CalendarCore
#    KF5::Codecs
#    KF5::AkonadiAgentBase
#    KF5::Contacts
#    KF5::AkonadiWidgets
#    KF5::I18n
#    KF5::KIOWidgets
#    )

