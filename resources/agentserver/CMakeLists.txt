set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

find_library(AGENT_SERVER_LIB akonadi_agent_server)

kde4_add_executable(akonadi_agent_server dummy_main.cpp)

target_link_libraries(akonadi_agent_server
  ${AGENT_SERVER_LIB}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${AKONADI_COMMON_LIBRARIES}
  akonadi_ical_resource
  akonadi_vcard_resource
  akonadi_maildir_resource
  akonadi_akonotes_resource

  akonadi_serializer_mail
  akonadi_serializer_kcalcore
  akonadi_serializer_contactgroup
  akonadi_serializer_addressee
)

install(TARGETS akonadi_agent_server
        DESTINATION ${BIN_INSTALL_DIR}
)
