include_directories(
  ${kdepim-runtime_SOURCE_DIR}
  ${Boost_INCLUDE_DIR}
  ${NEPOMUK_INCLUDES}
  )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )
add_definitions(-DNEPOMUK_RESOURCE_STATIC)

set( NEPOMUK_CONTACT_FEEDER_SRCS
  nepomukcalendarfeeder.cpp
)

soprano_add_ontology(NEPOMUK_CONTACT_FEEDER_SRCS
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/ncal.trig
  "NCAL"
  "Vocabulary"
  "trig")

kde4_add_executable(akonadi_nepomuk_calendar_feeder ${NEPOMUK_CONTACT_FEEDER_SRCS} ${NEPOMUKFEEDER_SHARED_SRCS} ${vocabulary_srcs})

target_link_libraries(akonadi_nepomuk_calendar_feeder
  ${NEPOMUKFEEDER_SHARED_LIBS}
  ${NEPOMUK_QUERY_LIBRARIES}
  ${KDEPIMLIBS_KCALCORE_LIBS}
)

if (Q_WS_MAC)
  set_target_properties(akonadi_nepomuk_calendar_feeder PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_nepomuk_calendar_feeder PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.calendarfeeder")
  set_target_properties(akonadi_nepomuk_calendar_feeder PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Nepomuk Calendar Feeder")
endif (Q_WS_MAC)

install(TARGETS akonadi_nepomuk_calendar_feeder ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES nepomukcalendarfeeder.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
