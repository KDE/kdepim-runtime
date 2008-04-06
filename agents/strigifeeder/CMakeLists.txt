include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

if ( STRIGIQTDBUSCLIENT_LIBRARY )

include_directories(${STRIGI_INCLUDE_DIR} ${STRIGI_INCLUDE_DIR}/strigi)

kde4_add_executable(akonadi_strigi_feeder RUN_UNINSTALLED strigifeeder.cpp)
target_link_libraries(akonadi_strigi_feeder
  ${KDE4_AKONADI_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${STRIGIQTDBUSCLIENT_LIBRARY}
  ${KDE4_KIO_LIBS}
)

install(TARGETS akonadi_strigi_feeder DESTINATION ${BIN_INSTALL_DIR})
install(FILES strigifeeder.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")

endif ( STRIGIQTDBUSCLIENT_LIBRARY )
