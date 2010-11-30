include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

option(BUILD_NEW_MAIL_NOTIFIER_AGENT FALSE "Build mobile new mail notification agent")
if (BUILD_NEW_MAIL_NOTIFIER_AGENT)

set(newmailnotifieragent_SRCS
  newmailnotifieragent.cpp
)

if (NOT WINCE)
  kde4_add_plugin(newmailnotifieragent ${newmailnotifieragent_SRCS})
else ()
  kde4_add_plugin(newmailnotifieragent STATIC ${newmailnotifieragent_SRCS})
endif ()

target_link_libraries(newmailnotifieragent
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
)

install(TARGETS newmailnotifieragent DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES newmailnotifieragent.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")

endif ()
