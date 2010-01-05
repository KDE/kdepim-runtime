set( invitationsagent_SRCS
  invitationsagent.cpp
  #outboxqueue.cpp
  #sendjob.cpp
  #storeresultjob.cpp
  ##configdialog.cpp
)

#kde4_add_ui_files(invitationsagent_SRCS settings.ui)
#kde4_add_kcfg_files(invitationsagent_SRCS settings.kcfgc)
#kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/invitationsagent.kcfg org.kde.Akonadi.MailDispatcher.Settings)
#qt4_add_dbus_adaptor(invitationsagent_SRCS
#  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.MailDispatcher.Settings.xml settings.h Settings
#)
## qt4_add_dbus_adaptor( invitationsagent_SRCS
## ${KDE4_DBUS_INTERFACES_DIR}/org.kde.Akonadi.MailDispatcher.xml invitationsagent.h MailDispatcherAgent
## )

kde4_add_executable(akonadi_invitations_agent ${invitationsagent_SRCS})

target_link_libraries(akonadi_invitations_agent
  ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS}
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${KDEPIMLIBS_MAILTRANSPORT_LIBS}
  ${KDEPIMLIBS_KCAL_LIBS}
  akonadi-kcal
  akonadi-kcal_next
)

install(TARGETS akonadi_invitations_agent ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES invitationsagent.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
