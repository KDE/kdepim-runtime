

set( invitationsagent_SRCS
  invitationsagent.cpp
  incidenceattribute.cpp
)

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_invitations_agent\")

#ki18n_wrap_ui(invitationsagent_SRCS settings.ui)
#kconfig_add_kcfg_files(invitationsagent_SRCS settings.kcfgc)
#kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/invitationsagent.kcfg org.kde.Akonadi.MailDispatcher.Settings)
#qt5_add_dbus_adaptor(invitationsagent_SRCS
#  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.MailDispatcher.Settings.xml settings.h Settings
#)
## qt5_add_dbus_adaptor( invitationsagent_SRCS
## ${KDE4_DBUS_INTERFACES_DIR}/org.kde.Akonadi.MailDispatcher.xml invitationsagent.h MailDispatcherAgent
## )

add_executable(akonadi_invitations_agent ${invitationsagent_SRCS})

target_link_libraries(akonadi_invitations_agent
  KF5::KIOCore
  KF5::AkonadiCore
  KF5::AkonadiMime
  KF5::Mime
  KF5::MailTransport
  KF5::CalendarCore
  KF5::AkonadiAgentBase
  KF5::IdentityManagement
)

if (Q_WS_MAC)
  set_target_properties(akonadi_invitations_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_invitations_agent PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.invitationsagent")
  set_target_properties(akonadi_invitations_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Invitations Calendar")
endif ()


install(TARGETS akonadi_invitations_agent ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES invitationsagent.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
