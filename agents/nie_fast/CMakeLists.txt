include(MacroAddOntologyClasses)
NEPOMUK_ADD_ONTOLOGY_CLASSES(
  nie_SRCS
  FAST
  ONTOLOGIES
  ${CMAKE_CURRENT_SOURCE_DIR}/../nie/nie.rdfs
  ${CMAKE_CURRENT_SOURCE_DIR}/../nie/nfo.rdfs
  ${CMAKE_CURRENT_SOURCE_DIR}/../nie/nco.rdfs
  ${CMAKE_CURRENT_SOURCE_DIR}/../nie/nmo.rdfs
  )
kde4_add_library(niefast STATIC ${nie_SRCS})
target_link_libraries(niefast ${QT_QTCORE_LIBRARY} ${NEPOMUK_LIBRARIES})
