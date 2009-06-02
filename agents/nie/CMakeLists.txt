include(MacroAddOntologyClasses)
NEPOMUK_ADD_ONTOLOGY_CLASSES(
  nie_SRCS
  ONTOLOGIES
  ${CMAKE_CURRENT_SOURCE_DIR}/nie.rdfs
  ${CMAKE_CURRENT_SOURCE_DIR}/nfo.rdfs
  ${CMAKE_CURRENT_SOURCE_DIR}/nco.rdfs
  ${CMAKE_CURRENT_SOURCE_DIR}/nmo.rdfs
  )
kde4_add_library(nie STATIC ${nie_SRCS})
target_link_libraries(nie ${QT_QTCORE_LIBRARY} ${NEPOMUK_LIBRARIES})
