#TODO: move the content of this file to a better location

include(NepomukAddOntologyClasses)
include_directories(${SOPRANO_INCLUDE_DIR})

nepomuk_add_ontology_classes(
  nie_SRCS
  FAST
  ONTOLOGIES
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nie.trig
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nco.trig
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nmo.trig
  ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/ncal.trig
  )

# generate nfo extra since otherwise we have two Attachments classes
nepomuk_add_ontology_classes(
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

kde4_add_library(niefast STATIC ${nie_SRCS})
target_link_libraries(niefast ${QT_QTCORE_LIBRARY} ${NEPOMUK_LIBRARIES})
