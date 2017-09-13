add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_invitations_agent\")


set( invitationsagent_SRCS
  invitationsagent.cpp
  incidenceattribute.cpp
)

ecm_qt_declare_logging_category(invitationsagent_SRCS HEADER invitationagent_debug.h IDENTIFIER INVITATIONAGENT_LOG CATEGORY_NAME org.kde.pim.invitationagent)

add_executable(akonadi_invitations_agent ${invitationsagent_SRCS})

target_link_libraries(akonadi_invitations_agent
  KF5::AkonadiCore
  KF5::AkonadiMime
  KF5::Mime
  KF5::CalendarCore
  KF5::AkonadiAgentBase
  KF5::I18n
)

if( APPLE )
  set_target_properties(akonadi_invitations_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_invitations_agent PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.invitationsagent")
  set_target_properties(akonadi_invitations_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Invitations Calendar")
endif ()


install(TARGETS akonadi_invitations_agent ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES invitationsagent.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
