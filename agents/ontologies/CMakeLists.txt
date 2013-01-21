project(akonadi_ontologies)

include(NepomukAddOntologyClasses)
include_directories(${SOPRANO_INCLUDE_DIR} ${NEPOMUK_CORE_INCLUDE_DIR})

# NepomukFeeder plugins won't link otherwise
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

configure_file(aneo.ontology.in ${CMAKE_CURRENT_BINARY_DIR}/aneo.ontology)
configure_file(nso.ontology.in ${CMAKE_CURRENT_BINARY_DIR}/nso.ontology)

install(FILES aneo.trig ${CMAKE_CURRENT_BINARY_DIR}/aneo.ontology DESTINATION ${CMAKE_INSTALL_PREFIX}/share/ontology/kde)
install(FILES nso.trig ${CMAKE_CURRENT_BINARY_DIR}/nso.ontology DESTINATION ${CMAKE_INSTALL_PREFIX}/share/ontology/kde)

soprano_add_ontology(nie_SRCS ${akonadi_ontologies_SOURCE_DIR}/aneo.trig "ANEO" "Vocabulary" "trig")
soprano_add_ontology(nie_SRCS ${akonadi_ontologies_SOURCE_DIR}/nso.trig "NSO" "Vocabulary" "trig")
soprano_add_ontology(nie_SRCS ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nie.trig "NIE" "Vocabulary" "trig")
soprano_add_ontology(nie_SRCS ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nco.trig "NCO" "Vocabulary" "trig")
soprano_add_ontology(nie_SRCS ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/nmo.trig "NMO" "Vocabulary" "trig")

kde4_add_library(niefast STATIC ${nie_SRCS})
target_link_libraries(niefast ${QT_QTCORE_LIBRARY} ${NEPOMUK_LIBRARIES} ${NEPOMUK_CORE_LIBRARY})
