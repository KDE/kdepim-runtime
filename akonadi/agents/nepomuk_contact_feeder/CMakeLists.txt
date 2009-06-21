include_directories(
  ${kdepim-runtime_SOURCE_DIR}
  ${Boost_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../nie_fast
  ${NEPOMUK_INCLUDES}
  )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

kde4_add_executable(akonadi_nepomuk_contact_feeder RUN_UNINSTALLED nepomukcontactfeeder.cpp)

target_link_libraries(akonadi_nepomuk_contact_feeder
  niefast
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${KDEPIMLIBS_KABC_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KDECORE_LIBS}
  )

install(TARGETS akonadi_nepomuk_contact_feeder ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES nepomukcontactfeeder.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
