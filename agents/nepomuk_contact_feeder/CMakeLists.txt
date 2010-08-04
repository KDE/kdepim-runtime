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

if (Q_WS_MAC)
  set_target_properties(akonadi_nepomuk_contact_feeder PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_nepomuk_contact_feeder PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.contactfeeder")
  set_target_properties(akonadi_nepomuk_contact_feeder PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Nepomuk Contact Feeder")
endif (Q_WS_MAC)

install(TARGETS akonadi_nepomuk_contact_feeder ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES nepomukcontactfeeder.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
