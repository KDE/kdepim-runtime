include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/libakonadi
	${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( mailthreaderagent_SRCS
  mailthreaderagent.cpp
  main.cpp
)

install( FILES mailthreaderagent.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/apps/akonadi/agents" )

kde4_automoc(${mailthreaderagent_SRCS})

kde4_add_executable(akonadi_mailthreader_agent RUN_UNINSTALLED ${mailthreaderagent_SRCS})

target_link_libraries(akonadi_mailthreader_agent akonadi ${QT_QTCORE_LIBRARIES} ${QT_QTDBUS_LIBRARIES} ${KDE4_KDECORE_LIBS} ${KDE4_KABC_LIBS} ${KDE4_KIO_LIBRARY})

install(TARGETS akonadi_mailthreader_agent DESTINATION ${BIN_INSTALL_DIR})
