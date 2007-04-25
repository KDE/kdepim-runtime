

include_directories(
        ${CMAKE_SOURCE_DIR}/akonadi
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


set(akonadi_serializer_addressee_PART_SRCS akonadi_serializer_addressee.cpp )
kde4_automoc(${akonadi_serializer_addressee_PART_SRCS})
kde4_add_plugin(akonadi_serializer_addressee WITH_PREFIX ${akonadi_serializer_addressee_PART_SRCS})
target_link_libraries(akonadi_serializer_addressee ${KDE4_KDECORE_LIBS} ${KDE4_KABC_LIBS} akonadi )
install(TARGETS akonadi_serializer_addressee DESTINATION ${PLUGIN_INSTALL_DIR})

set(akonadi_serializer_mail_PART_SRCS akonadi_serializer_mail.cpp )
kde4_automoc(${akonadi_serializer_mail_PART_SRCS})
kde4_add_plugin(akonadi_serializer_mail WITH_PREFIX ${akonadi_serializer_mail_PART_SRCS})
target_link_libraries(akonadi_serializer_mail ${KDE4_KDECORE_LIBS} ${KDE4_KMIME_LIBS} akonadi )
install(TARGETS akonadi_serializer_mail DESTINATION ${PLUGIN_INSTALL_DIR})




########### install files ###############

install( FILES akonadi_serializer_addressee.desktop akonadi_serializer_mail.desktop  DESTINATION ${DATA_INSTALL_DIR}/akonadi/plugins/serializer)

