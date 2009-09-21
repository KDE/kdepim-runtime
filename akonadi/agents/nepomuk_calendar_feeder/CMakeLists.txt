include_directories(
  ${kdepim-runtime_SOURCE_DIR}
  ${Boost_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../nie_fast
  ${NEPOMUK_INCLUDES}
  )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )
add_definitions(-DNEPOMUK_RESOURCE_STATIC)

# FIXME: currently the ontology2vocabularyclass tool generates invalid code on the NCAL ontology
#soprano_add_ontology(vocabulary_srcs ${CMAKE_CURRENT_SOURCE_DIR}/../nie_fast/ncal.rdfs "NCAL" "Vocabulary" "rdfxml")
set( vocabulary_srcs ncal.cpp )

kde4_add_executable(akonadi_nepomuk_calendar_feeder RUN_UNINSTALLED nepomukcalendarfeeder.cpp ${NEPOMUKFEEDER_SHARED_SRCS} ${vocabulary_srcs})

target_link_libraries(akonadi_nepomuk_calendar_feeder
  niefast
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${KDEPIMLIBS_KCAL_LIBS}
  ${NEPOMUK_LIBRARIES}
  ${SOPRANO_LIBRARIES}
  ${KDE4_KDECORE_LIBS}
  akonadi-kcal
  )

install(TARGETS akonadi_nepomuk_calendar_feeder ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES nepomukcalendarfeeder.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
