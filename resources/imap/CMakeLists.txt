set(imap_LIB_SRCS imaplib.cpp db.cpp socket.cpp)

kde4_add_library(imap SHARED ${imap_LIB_SRCS})

target_link_libraries(imap ${QT_AND_KDECORE_LIBS} ${QT_QTSQL_LIBRARY} ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KIMAP_LIBS})

set_target_properties(imap PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS imap DESTINATION ${LIB_INSTALL_DIR})

include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/libakonadi
	${CMAKE_SOURCE_DIR}/akonadi_agent
	${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( imaplibresource_SRCS
  imaplibresource.cpp
  main.cpp
)

install( FILES imaplibresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_executable(akonadi_imaplib_resource RUN_UNINSTALLED ${imaplibresource_SRCS})

target_link_libraries(akonadi_imaplib_resource akonadi ${QT_QTDBUS_LIBRARIES})

install(TARGETS akonadi_imaplib_resource DESTINATION ${BIN_INSTALL_DIR})

