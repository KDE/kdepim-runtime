add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_facebook_resource\")
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

set(fbresource_SRCS
    resource.cpp
    listjob.cpp
    eventslistjob.cpp
    birthdaylistjob.cpp
    settingsdialog.cpp
    tokenjobs.cpp
    graph.cpp
)

ki18n_wrap_ui(fbresource_SRCS
    settingsdialog.ui
)

ecm_qt_declare_logging_category(fbresource_SRCS HEADER resource_debug.h IDENTIFIER FBRESOURCE_LOG CATEGORY_NAME org.kde.pim.fbresource)

kcfg_generate_dbus_interface(settings.kcfg org.kde.Akonadi.Facebook.Settings )
kconfig_add_kcfg_files(fbresource_SRCS settings.kcfgc)

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
