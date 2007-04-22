

set(akonadi_serializer_addressee_PART_SRCS akonadi_serializer_addressee.cpp )

kde4_automoc(${akonadi_serializer_addressee_PART_SRCS})

kde4_add_plugin(akonadi_serializer_addressee WITH_PREFIX ${akonadi_serializer_addressee_PART_SRCS})



target_link_libraries(akonadi_serializer_addressee ${KDE4_KDECORE_LIBS} kabc akonadi )

install(TARGETS akonadi_serializer_addressee DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES akonadi_serializer_addressee.desktop  DESTINATION ${DATA_INSTALL_DIR}/akonadi/plugins/serializer)

