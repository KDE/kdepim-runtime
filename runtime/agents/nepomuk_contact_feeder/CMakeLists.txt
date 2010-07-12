include_directories(
  ${kdepim-runtime_SOURCE_DIR}
  ${Boost_INCLUDE_DIR}
  ${NEPOMUK_INCLUDES}
  )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )
add_definitions(-DNEPOMUK_RESOURCE_STATIC)

kde4_add_executable(akonadi_nepomuk_contact_feeder nepomukcontactfeeder.cpp ${NEPOMUKFEEDER_SHARED_SRCS} )

target_link_libraries(akonadi_nepomuk_contact_feeder
  ${KDEPIMLIBS_KABC_LIBS}
  ${NEPOMUKFEEDER_SHARED_LIBS}
)

install(TARGETS akonadi_nepomuk_contact_feeder ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES nepomukcontactfeeder.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
