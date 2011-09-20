
include_directories(
        ../
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

kde4_add_plugin( akonadi_nepomuk_note_feeder nepomuknotefeeder.cpp )
target_link_libraries( akonadi_nepomuk_note_feeder 
  ${NEPOMUKFEEDER_SHARED_LIBS}
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  nepomukfeederpluginlib
  akonadi-notes
)

install(
  FILES nepomuknotefeeder.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  TARGETS akonadi_nepomuk_note_feeder
  DESTINATION ${PLUGIN_INSTALL_DIR})

kde4_add_plugin( akonadi_nepomuk_calendar_feeder nepomukcalendarfeeder.cpp )
target_link_libraries( akonadi_nepomuk_calendar_feeder 
  ${NEPOMUKFEEDER_SHARED_LIBS}
  nepomukfeederpluginlib
  ${KDEPIMLIBS_KCALCORE_LIBS}
)

install(
  FILES nepomukcalendarfeeder.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  TARGETS akonadi_nepomuk_calendar_feeder
  DESTINATION ${PLUGIN_INSTALL_DIR})

kde4_add_plugin( akonadi_nepomuk_contact_feeder nepomukcontactfeeder.cpp )
target_link_libraries( akonadi_nepomuk_contact_feeder 
  ${NEPOMUKFEEDER_SHARED_LIBS}
  ${KDEPIMLIBS_KABC_LIBS}
  nepomukfeederpluginlib
)

install(
  FILES nepomukcontactfeeder.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})

install(
  TARGETS akonadi_nepomuk_contact_feeder
  DESTINATION ${PLUGIN_INSTALL_DIR})





