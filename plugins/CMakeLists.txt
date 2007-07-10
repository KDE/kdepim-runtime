add_subdirectory( tests )

include_directories(
        ${CMAKE_SOURCE_DIR}/akonadi
        ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


set(akonadi_serializer_addressee_PART_SRCS akonadi_serializer_addressee.cpp )
kde4_add_plugin(akonadi_serializer_addressee WITH_PREFIX ${akonadi_serializer_addressee_PART_SRCS})
target_link_libraries(akonadi_serializer_addressee ${KDE4_KDECORE_LIBS} ${KDE4_KABC_LIBS} akonadi )
install(TARGETS akonadi_serializer_addressee DESTINATION ${PLUGIN_INSTALL_DIR})

set(akonadi_serializer_mail_PART_SRCS akonadi_serializer_mail.cpp )
kde4_add_plugin(akonadi_serializer_mail WITH_PREFIX ${akonadi_serializer_mail_PART_SRCS})
target_link_libraries(akonadi_serializer_mail ${KDE4_KDECORE_LIBS} ${KDE4_KMIME_LIBS} akonadi )
install(TARGETS akonadi_serializer_mail DESTINATION ${PLUGIN_INSTALL_DIR})

set(akonadi_serializer_kcal_SRCS akonadi_serializer_kcal.cpp)
kde4_add_plugin(akonadi_serializer_kcal WITH_PREFIX ${akonadi_serializer_kcal_SRCS})
target_link_libraries(akonadi_serializer_kcal ${KDE4_KDECORE_LIBS} ${KDE4_KCAL_LIBS} akonadi)
install(TARGETS akonadi_serializer_kcal DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install( FILES
  akonadi_serializer_addressee.desktop
  akonadi_serializer_mail.desktop
  akonadi_serializer_kcal.desktop
DESTINATION ${DATA_INSTALL_DIR}/akonadi/plugins/serializer)

