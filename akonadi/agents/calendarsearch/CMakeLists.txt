include_directories(
  ${kdepim-runtime_SOURCE_DIR}
  ${Boost_INCLUDE_DIR}
)

set( CALENDARSEARCHAGENT_SRCS calendarsearchagent.cpp )
qt4_add_dbus_adaptor(CALENDARSEARCHAGENT_SRCS org.freedesktop.Akonadi.CalendarSearchAgent.xml calendarsearchagent.h CalendarSearchAgent )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

kde4_add_executable(akonadi_calendarsearch_agent ${CALENDARSEARCHAGENT_SRCS})

target_link_libraries(akonadi_calendarsearch_agent
  ${KDEPIMLIBS_KCAL_LIBS}
  akonadi-kcal
)

install(TARGETS akonadi_calendarsearch_agent ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES calendarsearchagent.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
