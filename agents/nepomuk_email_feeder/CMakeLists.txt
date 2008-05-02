include_directories(
  ${CMAKE_SOURCE_DIR}/akonadi
  ${CMAKE_CURRENT_BINARY_DIR}/../nie
  ${Boost_INCLUDE_DIR}
  )

if(HAVE_NIE)
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

  set(akonadi_nepomuk_email_feeder_SRCS nepomukemailfeeder.cpp)

  kde4_add_app_icon(akonadi_nepomuk_email_feeder_SRCS "${KDE4_ICON_DIR}/oxygen/*/apps/nepomuk.png")

  kde4_add_executable(akonadi_nepomuk_email_feeder RUN_UNINSTALLED ${akonadi_nepomuk_email_feeder_SRCS})

  target_link_libraries(akonadi_nepomuk_email_feeder
    nie
    ${KDE4_AKONADI_LIBS}
    akonadi-kmime
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${KDE4_KMIME_LIBS}
    ${NEPOMUK_LIBRARIES}
    )

  install(TARGETS akonadi_nepomuk_email_feeder DESTINATION ${BIN_INSTALL_DIR})
  install(FILES nepomukemailfeeder.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
endif(HAVE_NIE)
