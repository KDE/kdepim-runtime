include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

if ( STRIGIQTDBUSCLIENT_LIBRARY )

include_directories( ${STRIGI_INCLUDE_DIR} ${STRIGI_INCLUDE_DIR}/strigi )

set( mailthreaderagent_SRCS
  mailthreaderagent.cpp
)


kde4_add_executable(akonadi_mailthreader_agent RUN_UNINSTALLED ${mailthreaderagent_SRCS})

target_link_libraries(akonadi_mailthreader_agent ${KDE4_AKONADI_LIBS} ${KDE4_AKONADI_KMIME_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY}  ${STRIGIQTDBUSCLIENT_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KABC_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KMIME_LIBS} )

install(TARGETS akonadi_mailthreader_agent ${INSTALL_TARGETS_DEFAULT_ARGS})
install( FILES mailthreaderagent.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

endif ( STRIGIQTDBUSCLIENT_LIBRARY )
