include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/libakonadi
	${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

kde4_add_executable(akonadi_nepomuk_feeder RUN_UNINSTALLED nepomukfeeder.cpp)

target_link_libraries(akonadi_nepomuk_feeder
  akonadi
  ${QT_QTCORE_LIBRARIES}
  ${QT_QTDBUS_LIBRARIES}
  ${KDE4_KMIME_LIBS}
  ${NEPOMUK_LIBRARIES}
)

install(TARGETS akonadi_nepomuk_feeder DESTINATION ${BIN_INSTALL_DIR})
install(FILES nepomukfeeder.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
