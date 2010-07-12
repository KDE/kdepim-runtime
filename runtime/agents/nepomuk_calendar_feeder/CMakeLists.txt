include_directories(
  ${kdepim-runtime_SOURCE_DIR}
  ${Boost_INCLUDE_DIR}
  ${NEPOMUK_INCLUDES}
  )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )
add_definitions(-DNEPOMUK_RESOURCE_STATIC)

# FIXME: currently the ontology2vocabularyclass tool generates invalid code on the NCAL ontology
soprano_add_ontology(vocabulary_srcs ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nie/ncal.trig "NCAL" "Vocabulary" "trig")
set( vocabulary_srcs ncal.cpp )

kde4_add_executable(akonadi_nepomuk_calendar_feeder nepomukcalendarfeeder.cpp ${NEPOMUKFEEDER_SHARED_SRCS} ${vocabulary_srcs})

target_link_libraries(akonadi_nepomuk_calendar_feeder
  ${NEPOMUKFEEDER_SHARED_LIBS}
  ${KDEPIMLIBS_AKONADI_KCAL_LIBS}
  ${KDEPIMLIBS_KCAL_LIBS}
)

install(TARGETS akonadi_nepomuk_calendar_feeder ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES nepomukcalendarfeeder.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
