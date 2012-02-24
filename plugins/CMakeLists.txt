add_subdirectory( tests )

set(akonadi_serializer_rss_SRCS
    akonadi_serializer_rss.cpp
)

if(KRSS_ENABLE_PROTOBUF_SERIALIZER)
    WRAP_PROTO(krss_LIB_PROTOBUF_SRCS krss.proto)

    set(akonadi_serializer_rss_SRCS
        ${akonadi_serializer_rss_SRCS}
        ${krss_LIB_PROTOBUF_SRCS}
        rssitemserializer_protobuf.cpp
    )
    set(KRSS_EXTRA_LIBS ${PROTOBUF_LIBRARY})
else(KRSS_ENABLE_PROTOBUF_SERIALIZER)
if(KRSS_ENABLE_QDATASTREAM_SERIALIZER)
    set(akonadi_serializer_rss_SRCS
        ${akonadi_serializer_rss_SRCS}
        rssitemserializer_qdatastream.cpp
    )
    set(KRSS_EXTRA_LIBS ${PROTOBUF_LIBRARY})
else(KRSS_ENABLE_QDATASTREAM_SERIALIZER)
if(KRSS_ENABLE_JSON_SERIALIZER)
   set(akonadi_serializer_rss_SRCS
       ${akonadi_serializer_rss_SRCS}
       rssitemserializer_json.cpp
   )
   set(KRSS_EXTRA_LIBS qjson)
else(KRSS_ENABLE_JSON_SERIALIZER)
   set(akonadi_serializer_rss_SRCS
       ${akonadi_serializer_rss_SRCS}
       rssitemserializer_xml.cpp
   )
endif(KRSS_ENABLE_JSON_SERIALIZER)
endif(KRSS_ENABLE_QDATASTREAM_SERIALIZER)
endif(KRSS_ENABLE_PROTOBUF_SERIALIZER)

kde4_add_plugin(akonadi_serializer_rss ${akonadi_serializer_rss_SRCS})
target_link_libraries(akonadi_serializer_rss ${KDE4_KIO_LIBS} ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_SYNDICATION_LIBS} krss)

install(TARGETS akonadi_serializer_rss DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES akonadi_serializer_rss.desktop DESTINATION "${DATA_INSTALL_DIR}/akonadi/plugins/serializer")
