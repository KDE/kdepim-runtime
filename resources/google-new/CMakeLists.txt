add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_google_resource\")

set(googleresource_SRCS
    googleresource.cpp
    googleaccountmanager.cpp
    googlesettings.cpp
    googlesettingsdialog.cpp
    kgapiversionattribute.cpp
    defaultreminderattribute.cpp

    generichandler.cpp
    calendarhandler.cpp
    taskhandler.cpp
    contacthandler.cpp
    ${accounts_SRCS})

if (ECM_VERSION VERSION_LESS "5.68.0")
    ecm_qt_declare_logging_category(googleresource_SRCS HEADER googleresource_debug.h IDENTIFIER GOOGLE_LOG CATEGORY_NAME org.kde.pim.google)
else()
    ecm_qt_declare_logging_category(googleresource_SRCS HEADER googleresource_debug.h IDENTIFIER GOOGLE_LOG CATEGORY_NAME org.kde.pim.google
        DESCRIPTION "resource google (kdepim-runtime)"
        EXPORT KDEPIMRUNTIME
    )
endif()

ki18n_wrap_ui(googleresource_SRCS googlesettingsdialog.ui)

kconfig_add_kcfg_files(googleresource_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfgc)

kcfg_generate_dbus_interface(
  ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfg
  org.kde.Akonadi.Google.Settings
)

qt5_add_dbus_adaptor(googleresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Google.Settings.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/googlesettings.h GoogleSettings
)

add_executable(akonadi_google_resource ${googleresource_SRCS})

if( APPLE )
  set_target_properties(akonadi_google_resource PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../../Info.plist.template
  )
  set_target_properties(akonadi_google_resource PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.google"
  )
  set_target_properties(akonadi_google_resource PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Google Resource"
  )
endif()

target_link_libraries(akonadi_google_resource
  KF5::AkonadiCalendar
  KF5::AkonadiCore
  KF5::AkonadiAgentBase
  KF5::CalendarCore
  KF5::Contacts
  KF5::Wallet
  KF5::I18n
  KF5::WindowSystem
  KF5::Completion
  KF5::TextWidgets
  KPim::GAPICalendar
  KPim::GAPIContacts
  KPim::GAPICore
  KPim::GAPITasks
)

install(TARGETS akonadi_google_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(
  FILES googleresource.desktop
  DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents"
)
