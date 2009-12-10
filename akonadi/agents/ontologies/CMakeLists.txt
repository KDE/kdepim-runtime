include(NepomukAddOntologyClasses)
include_directories(${SOPRANO_INCLUDE_DIR})

#configure_file(nrss.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/nrss.desktop)
#install(FILES nrss.rdfs ${CMAKE_CURRENT_BINARY_DIR}/nrss.desktop
#        DESTINATION ${DATA_INSTALL_DIR}/nepomuk/ontologies
#)

if(SHAREDDESKTOPONTOLOGIES_VERSION VERSION_LESS 0.2)
  NEPOMUK_ADD_ONTOLOGY_CLASSES(
    nie_SRCS
    FAST
    ONTOLOGIES
    ${CMAKE_CURRENT_SOURCE_DIR}/nie.rdfs
    ${CMAKE_CURRENT_SOURCE_DIR}/nco.rdfs
    ${CMAKE_CURRENT_SOURCE_DIR}/nmo.rdfs
    ${CMAKE_CURRENT_SOURCE_DIR}/ncal.rdfs
    ${CMAKE_CURRENT_SOURCE_DIR}/nrss.rdfs
    )

  # generate nfo extra since otherwise we have two Attachments classes
  NEPOMUK_ADD_ONTOLOGY_CLASSES(
    nie_SRCS
    FAST
    ONTOLOGIES
    ${CMAKE_CURRENT_SOURCE_DIR}/nfo.rdfs
    CLASSES
    "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Website"
    )

  soprano_add_ontology(nie_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/nie.rdfs "NIE" "Vocabulary" "rdfxml")
  soprano_add_ontology(nie_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/nco.rdfs "NCO" "Vocabulary" "rdfxml")
  soprano_add_ontology(nie_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/nmo.rdfs "NMO" "Vocabulary" "rdfxml")
else(SHAREDDESKTOPONTOLOGIES_VERSION VERSION_LESS 0.2)
  NEPOMUK_ADD_ONTOLOGY_CLASSES(
    nie_SRCS
    FAST
    ONTOLOGIES
    ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nie.trig
    ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nco.trig
    ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nmo.trig
    ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/ncal.trig
    ${CMAKE_CURRENT_SOURCE_DIR}/nrss.rdfs
    )

  # generate nfo extra since otherwise we have two Attachments classes
  NEPOMUK_ADD_ONTOLOGY_CLASSES(
    nie_SRCS
    FAST
    ONTOLOGIES
    ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nfo.trig
    CLASSES
    "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Website"
    )

  soprano_add_ontology(nie_SRCS ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nie.trig "NIE" "Vocabulary" "trig")
  soprano_add_ontology(nie_SRCS ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nco.trig "NCO" "Vocabulary" "trig")
  soprano_add_ontology(nie_SRCS ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nmo.trig "NMO" "Vocabulary" "trig")
endif(SHAREDDESKTOPONTOLOGIES_VERSION VERSION_LESS 0.2)

soprano_add_ontology(nie_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/nrss.rdfs "NRSS" "Vocabulary" "rdfxml")

kde4_add_library(niefast STATIC ${nie_SRCS})
target_link_libraries(niefast ${QT_QTCORE_LIBRARY} ${NEPOMUK_LIBRARIES})
