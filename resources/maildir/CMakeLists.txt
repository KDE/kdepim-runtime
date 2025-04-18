
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/libmaildir
    ${kdepim-runtime_SOURCE_DIR}
)


if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()

# maildir access library
add_subdirectory(libmaildir)

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_maildir_resource\")

set(maildir_common_SRCS)
kconfig_add_kcfg_files(maildir_common_SRCS settings.kcfgc)

############################### Resource #####################################
set(maildirresource_SRCS
  main.cpp
  maildirresource.cpp
  retrieveitemsjob.cpp
  maildirresource.h
  retrieveitemsjob.h
  ${maildir_common_SRCS}
)

ecm_qt_declare_logging_category(maildirresource_SRCS HEADER maildirresource_debug.h IDENTIFIER MAILDIRRESOURCE_LOG CATEGORY_NAME org.kde.pim.maildirresource
        DESCRIPTION "maildir resource (kdepim-runtime)"
        OLD_CATEGORY_NAMES log_maildirresource
        EXPORT KDEPIMRUNTIME
)


kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/maildirresource.kcfg org.kde.Akonadi.Maildir.Settings)

qt_add_dbus_adaptor(maildirresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Maildir.Settings.xml settings.h Akonadi_Maildir_Resource::MaildirSettings maildirsettingsadaptor MaildirSettingsAdaptor
)

add_executable(akonadi_maildir_resource ${maildirresource_SRCS})
if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_maildir_resource PROPERTIES UNITY_BUILD ON)
endif()
target_link_libraries(akonadi_maildir_resource
    maildir
    KPim6::AkonadiCore
    KPim6::AkonadiAgentBase
    KPim6::AkonadiMime
    KPim6::Mime
    KF6::I18n
    KF6::ConfigGui
)

install(TARGETS akonadi_maildir_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Maildir.Settings.xml
        DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})

install(FILES maildirresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")

############################# Config plugin ###############################
kcoreaddons_add_plugin(maildirconfig
    INSTALL_NAMESPACE "pim6/akonadi/config"
)

target_sources(maildirconfig PRIVATE
    maildirconfigmain.cpp
    maildirconfig.cpp
    configwidget.cpp
    maildirconfig.h
    configwidget.h
    ${maildir_common_SRCS}
)

ki18n_wrap_ui(maildirconfig settings.ui)


target_link_libraries(maildirconfig
    KPim6::AkonadiCore
    KF6::ConfigWidgets
    KF6::KIOWidgets
    KF6::I18n
    folderarchivesettings
    maildir
)

if(HAVE_ACTIVITY_SUPPORT)
    target_link_libraries(maildirconfig
        KPim6::PimCommonActivities
    )
endif()
