include_directories(
  ${kdepim-runtime_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../nie_fast
  ${Boost_INCLUDE_DIR}
  )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )
add_definitions(-DNEPOMUK_RESOURCE_STATIC)

if ( STRIGIQTDBUSCLIENT_LIBRARY )

set(akonadi_nepomuk_email_feeder_SRCS
  nepomukemailfeeder.cpp
  messageanalyzer.cpp
  configdialog.cpp
  ${NEPOMUKFEEDER_SHARED_SRCS}
)

kde4_add_app_icon(akonadi_nepomuk_email_feeder_SRCS "${KDE4_ICON_DIR}/oxygen/*/apps/nepomuk.png")
kde4_add_kcfg_files(akonadi_nepomuk_email_feeder_SRCS settings.kcfgc)
kde4_add_ui_files(akonadi_nepomuk_email_feeder_SRCS configdialog.ui)

kde4_add_executable(akonadi_nepomuk_email_feeder ${akonadi_nepomuk_email_feeder_SRCS})

target_link_libraries(akonadi_nepomuk_email_feeder
  ${NEPOMUKFEEDER_SHARED_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
)

install(TARGETS akonadi_nepomuk_email_feeder ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES nepomukemailfeeder.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")

endif ( STRIGIQTDBUSCLIENT_LIBRARY )
