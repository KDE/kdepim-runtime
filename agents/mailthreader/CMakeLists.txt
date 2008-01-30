include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/libakonadi
	${QT_QTDBUS_INCLUDE_DIR}
	${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

if ( STRIGIQTDBUSCLIENT_LIBRARY )

include_directories( ${STRIGI_INCLUDE_DIR} ${STRIGI_INCLUDE_DIR}/strigi )

set( mailthreaderagent_SRCS
  mailthreaderagent.cpp
  main.cpp
)


kde4_add_executable(akonadi_mailthreader_agent RUN_UNINSTALLED ${mailthreaderagent_SRCS})

kdepim4_link_unique_libraries(akonadi_mailthreader_agent akonadi ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY}  ${STRIGIQTDBUSCLIENT_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KABC_LIBS} ${KDE4_KIO_LIBRARY} ${KDE4_KMIME_LIBS} )

install(TARGETS akonadi_mailthreader_agent DESTINATION ${BIN_INSTALL_DIR})
install( FILES mailthreaderagent.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

endif ( STRIGIQTDBUSCLIENT_LIBRARY )
