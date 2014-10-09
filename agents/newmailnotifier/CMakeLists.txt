include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )
add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )



set(newmailnotifieragent_SRCS
  newmailnotifiersettingsdialog.cpp
  newmailnotifieragent.cpp
  newmailnotifierattribute.cpp
  specialnotifierjob.cpp
  newmailnotifierselectcollectionwidget.cpp
  newmailnotifiershowmessagejob.cpp
  util.cpp
)

kde4_add_kcfg_files(newmailnotifieragent_SRCS
    newmailnotifieragentsettings.kcfgc
  )


qt4_add_dbus_adaptor(newmailnotifieragent_SRCS org.freedesktop.Akonadi.NewMailNotifier.xml newmailnotifieragent.h NewMailNotifierAgent)


kde4_add_executable( akonadi_newmailnotifier_agent ${newmailnotifieragent_SRCS})


target_link_libraries( akonadi_newmailnotifier_agent
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${KDE4_KNOTIFYCONFIG_LIBS}
  ${KDEPIMLIBS_KABC_LIBS}
  ${KDEPIMLIBS_KPIMUTILS_LIBS}
  ${KDEPIMLIBS_AKONADI_CONTACT_LIBS}
  ${KDEPIMLIBS_KPIMIDENTITIES_LIBS}
)


if (Q_WS_MAC)
  set_target_properties( akonadi_newmailnotifier_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist.template)
  set_target_properties( akonadi_newmailnotifier_agent PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.newmailnotifier")
  set_target_properties( akonadi_newmailnotifier_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE New Mail Notifier")
endif ()

install(TARGETS akonadi_newmailnotifier_agent ${INSTALL_TARGETS_DEFAULT_ARGS} )


install(FILES newmailnotifieragent.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
install(FILES akonadi_newmailnotifier_agent.notifyrc DESTINATION "${DATA_INSTALL_DIR}/akonadi_newmailnotifier_agent" )

add_subdirectory(kconf_update)
