add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_outlook_resource\")

# The cxx bindgen producs macros with "$" in the name, which Clang complains about
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-dollar-in-identifier-extension")
endif()

corrosion_import_crate(
  MANIFEST_PATH lib/Cargo.toml
  TYPES staticlib
)

set(CXXQT_EXPORT_DIR "${CMAKE_CURRENT_BINARY_DIR}/cxxqt")
get_target_property(QMAKE Qt::qmake IMPORTED_LOCATION)
corrosion_set_env_vars(libgraph
    "CXXQT_EXPORT_DIR=${CXXQT_EXPORT_DIR}"
    "QMAKE=${QMAKE}"
)


set(resource_common_SRCS
    outlooksettings.cpp
    outlooksettings.h
)

ecm_qt_declare_logging_category(resource_common_SRCS
    HEADER outlookresource_debug.h
    IDENTIFIER OUTLOOK_LOG
    CATEGORY_NAME org.kde.pim.outlook
    DESCRIPTION "resource Outlook (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME)

kconfig_add_kcfg_files(resource_common_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfgc)

kcfg_generate_dbus_interface(
  ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfg
  org.kde.Akonadi.Outlook.Settings
)

qt_add_dbus_adaptor(resource_common_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Outlook.Settings.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/outlooksettings.h OutlookSettings
)

############################# Resource #################################
set(outlook_resource_SRCS
    outlookresource.cpp
    conversion.cpp
    ${resource_common_SRCS}
)

add_executable(akonadi_outlook_resource ${outlook_resource_SRCS})
target_include_directories(akonadi_outlook_resource PRIVATE "${CXXQT_EXPORT_DIR}/libgraph")

if(APPLE)
  set_target_properties(akonadi_outlook_resource PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../../Info.plist.template
)
  set_target_properties(akonadi_outlook_resource PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.outlook"
)
  set_target_properties(akonadi_outlook_resource PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Outlook Resource"
)
endif()

target_link_libraries(akonadi_outlook_resource
  KPim6::AkonadiCalendar
  KPim6::AkonadiCore
  KPim6::AkonadiAgentBase
  KF6::CalendarCore
  KF6::Contacts
  KF6::I18n
  KF6::WindowSystem
  KF6::Notifications
  qt6keychain
  libgraph
  OpenSSL::SSL
  OpenSSL::Crypto
)

install(TARGETS akonadi_outlook_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(
  FILES outlookresource.desktop
  DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents"
)

install(
    FILES org.kde.akonadi_outlook_resource.desktop
    DESTINATION ${KDE_INSTALL_APPDIR}
)

#install(
#  FILES akonadi_outlook_resource.notifyrc
#  DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR}
#)

################################ Config plugin ###############################

kcoreaddons_add_plugin(outlookconfig
    INSTALL_NAMESPACE "pim${QT_MAJOR_VERSION}/akonadi/config"
)

target_sources(outlookconfig
PRIVATE
    outlookconfig.cpp
    outlooksettingswidget.ui
    outlooksettingswidget.cpp
    outlooksettingswidget.h
    ${resource_common_SRCS}
)

ki18n_wrap_ui(outlookconfig outlooksettingswidget.ui)

target_link_libraries(outlookconfig
    KPim6::AkonadiCore
    KPim6::AkonadiCalendar
    KPim6::AkonadiAgentBase
    KF6::CalendarCore
    KF6::Contacts
    KF6::Wallet
    KF6::I18n
    KF6::WindowSystem
    KF6::Notifications
    qt6keychain
)

