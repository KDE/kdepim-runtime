
#########################################################################
# AKONADI SERIALIZER SECTION
#########################################################################

set(akonadi_serializer_kjots_PART_SRCS
  akonadi_serializer_kjots.cpp
)

kde4_add_plugin( akonadi_serializer_kjots ${akonadi_serializer_kjots_PART_SRCS} )
target_link_libraries(akonadi_serializer_kjots
                      kjots_common
                      ${QT_QTXML_LIBRARY}
                      ${QT_QTCORE_LIBRARY}
)

install(TARGETS akonadi_serializer_kjots
        DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES akonadi_serializer_kjots.desktop
        DESTINATION ${DATA_INSTALL_DIR}/akonadi/plugins/serializer)

