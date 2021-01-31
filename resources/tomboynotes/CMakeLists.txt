add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_tomboynotes_resource\")


########### next target ###############

set(tomboynotesresource_common_SRCS)

kconfig_add_kcfg_files(tomboynotesresource_common_SRCS
    settings.kcfgc
    )

set(tomboynotesresource_SRCS
    tomboynotesresource.cpp
    tomboycollectionsdownloadjob.cpp
    tomboyitemdownloadjob.cpp
    tomboyitemuploadjob.cpp
    tomboyitemsdownloadjob.cpp
    tomboyjobbase.cpp
    tomboyserverauthenticatejob.cpp
    o1tomboy.cpp
    o2/o0settingsstore.cpp
    o2/o0baseauth.cpp
    o2/o0abstractstore.h
    o2/o0globals.h
    o2/o1.cpp
    o2/o1requestor.cpp
    o2/o1timedreply.cpp
    o2/o2.cpp
    o2/o2reply.cpp
    o2/o2replyserver.cpp
    o2/o2requestor.cpp
    o2/o2simplecrypt.cpp
    )

ecm_qt_declare_logging_category(tomboynotesresource_SRCS
    HEADER debug.h
    IDENTIFIER TOMBOYNOTESRESOURCE_LOG
    CATEGORY_NAME org.kde.pim.tomboynotesresource
    DESCRIPTION "tomboynote resource  (kdepim-runtime)"
    OLD_CATEGORY_NAMES log_tomboynotesresource
    EXPORT KDEPIMRUNTIME
    )




kconfig_add_kcfg_files(tomboynotesresource_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/settings.kcfgc
    )

kcfg_generate_dbus_interface(
    ${CMAKE_CURRENT_SOURCE_DIR}/tomboynotesresource.kcfg
    org.kde.Akonadi.TomboyNotes.Settings
    )

qt5_add_dbus_adaptor(tomboynotesresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.TomboyNotes.Settings.xml
    ${CMAKE_CURRENT_BINARY_DIR}/settings.h
    Settings
    )


add_executable(akonadi_tomboynotes_resource ${tomboynotesresource_SRCS})
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_tomboynotes_resource PROPERTIES UNITY_BUILD ON)
endif()
target_link_libraries(akonadi_tomboynotes_resource
    Qt::DBus
    Qt::Gui
    Qt::Network
    Qt::WebEngineWidgets
    KF5::AkonadiAgentBase
    KF5::AkonadiNotes
    KF5::ConfigCore
    KF5::ConfigWidgets
    KF5::I18n
    KF5::KIOCore
    KF5::KIOFileWidgets
    KF5::KIONTLM
    KF5::KIOWidgets
    KF5::Mime
    )

install(TARGETS akonadi_tomboynotes_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES tomboynotesresource.desktop
    DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents"
    )

############################# Config plugin ################################
set(tomboynotesconfig_ui_SRCS)
ki18n_wrap_ui(tomboynotesconfig_ui_SRCS tomboynotesagentconfigwidget.ui)
set(tomboynotesconfig_SRCS
    tomboynotesconfigwidget.cpp
    ${tomboynotesconfig_ui_SRCS}
    ${tomboynotesresource_common_SRCS}
    )


kcoreaddons_add_plugin(tomboynotesconfig
    SOURCES ${tomboynotesconfig_SRCS}
    JSON "tomboynotesconfig.json"
    INSTALL_NAMESPACE "akonadi/config"
    )
target_link_libraries(tomboynotesconfig
    KF5::AkonadiCore
    KF5::CalendarCore
    KF5::Contacts
    KF5::AkonadiWidgets
    KF5::I18n
    KF5::ConfigWidgets
    KF5::Completion
    )
