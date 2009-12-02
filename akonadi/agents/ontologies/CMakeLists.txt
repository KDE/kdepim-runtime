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
  ${CMAKE_CURRENT_SOURCE_DIR}/nrss.rdfs
  )

soprano_add_ontology(nie_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/nie.rdfs "NIE" "Vocabulary" "rdfxml")
soprano_add_ontology(nie_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/nco.rdfs "NCO" "Vocabulary" "rdfxml")
soprano_add_ontology(nie_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/nmo.rdfs "NMO" "Vocabulary" "rdfxml")
soprano_add_ontology(nie_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/nrss.rdfs "NRSS" "Vocabulary" "rdfxml")

#configure_file(nrss.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/nrss.desktop)
#install(FILES nrss.rdfs ${CMAKE_CURRENT_BINARY_DIR}/nrss.desktop
#        DESTINATION ${DATA_INSTALL_DIR}/nepomuk/ontologies
#)

kde4_add_library(niefast STATIC ${nie_SRCS})
target_link_libraries(niefast ${QT_QTCORE_LIBRARY} ${NEPOMUK_LIBRARIES})
