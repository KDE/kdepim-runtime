project(akonadi_serializer_qimage)


set(akonadi_serializer_qimage_SRCS
  akonadi_serializer_qimage.cpp
)

include_directories (
  ${CMAKE_PROJECT_BINARY_DIR}/
)


kde4_add_plugin( akonadi_serializer_qimage ${akonadi_serializer_qimage_SRCS} )
target_link_libraries(akonadi_serializer_qimage
                      ${AKONADI_COMMON_LIBRARIES}
                      ${KDEPIMLIBS_AKONADI_LIBS}
                      ${QT_QTCORE_LIBRARY}
                      ${QT_QTGUI_LIBRARY}
)

install(TARGETS akonadi_serializer_qimage
        DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES akonadi_serializer_qimage.desktop
        DESTINATION ${DATA_INSTALL_DIR}/akonadi/plugins/serializer)

