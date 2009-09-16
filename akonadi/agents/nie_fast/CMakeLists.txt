include(NepomukAddOntologyClasses)
NEPOMUK_ADD_ONTOLOGY_CLASSES(
  nie_SRCS
  FAST
  ONTOLOGIES
  ${CMAKE_CURRENT_SOURCE_DIR}/nie.rdfs
  ${CMAKE_CURRENT_SOURCE_DIR}/nfo.rdfs
  ${CMAKE_CURRENT_SOURCE_DIR}/nco.rdfs
  ${CMAKE_CURRENT_SOURCE_DIR}/nmo.rdfs
  ${CMAKE_CURRENT_SOURCE_DIR}/ncal.rdfs
  )

kde4_add_library(niefast STATIC ${nie_SRCS})
target_link_libraries(niefast ${QT_QTCORE_LIBRARY} ${NEPOMUK_LIBRARIES})
