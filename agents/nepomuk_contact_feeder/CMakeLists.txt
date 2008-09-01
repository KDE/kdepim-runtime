include_directories(
  ${CMAKE_SOURCE_DIR}/akonadi
  ${Boost_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../nie
  ${NEPOMUK_INCLUDES}
  )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

kde4_add_executable(akonadi_nepomuk_contact_feeder RUN_UNINSTALLED nepomukcontactfeeder.cpp)

target_link_libraries(akonadi_nepomuk_contact_feeder
  nie
  ${KDE4_AKONADI_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${KDE4_KABC_LIBS}
  ${NEPOMUK_LIBRARIES}
  )

install(TARGETS akonadi_nepomuk_contact_feeder ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES nepomukcontactfeeder.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
