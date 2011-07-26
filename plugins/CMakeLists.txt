set(akonadi_serializer_rss_SRCS
    akonadi_serializer_rss.cpp
)

kde4_add_plugin(akonadi_serializer_rss ${akonadi_serializer_rss_SRCS})
target_link_libraries(akonadi_serializer_rss ${KDE4_KIO_LIBS} ${KDEPIMLIBS_AKONADI_LIBS} krss)

install(TARGETS akonadi_serializer_rss DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES akonadi_serializer_rss.desktop DESTINATION "${DATA_INSTALL_DIR}/akonadi/plugins/serializer")
