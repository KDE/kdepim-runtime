add_subdirectory( tests )

include_directories(
        ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


set(akonadi_serializer_addressee_PART_SRCS akonadi_serializer_addressee.cpp )
kde4_add_plugin(akonadi_serializer_addressee ${akonadi_serializer_addressee_PART_SRCS})
target_link_libraries(akonadi_serializer_addressee ${KDE4_KDECORE_LIBS} ${KDE4_KABC_LIBS} ${KDE4_AKONADI_LIBS} akonadi-kabc ${KDE4_AKONADI_KABC_LIBS})
install(TARGETS akonadi_serializer_addressee DESTINATION ${PLUGIN_INSTALL_DIR})

set(akonadi_serializer_mail_PART_SRCS akonadi_serializer_mail.cpp )
kde4_add_plugin(akonadi_serializer_mail ${akonadi_serializer_mail_PART_SRCS})
target_link_libraries(akonadi_serializer_mail ${KDE4_KDECORE_LIBS} ${KDE4_KMIME_LIBS} ${KDE4_AKONADI_LIBS} ${KDE4_AKONADI_KMIME_LIBS} ${AKONADI_COMMON_LIBRARIES} )
install(TARGETS akonadi_serializer_mail DESTINATION ${PLUGIN_INSTALL_DIR})

set(akonadi_serializer_kcal_SRCS akonadi_serializer_kcal.cpp)
kde4_add_plugin(akonadi_serializer_kcal ${akonadi_serializer_kcal_SRCS})
target_link_libraries(akonadi_serializer_kcal ${KDE4_KDECORE_LIBS} ${KDE4_KCAL_LIBS} ${KDE4_AKONADI_LIBS})
install(TARGETS akonadi_serializer_kcal DESTINATION ${PLUGIN_INSTALL_DIR})

set(akonadi_serializer_bookmark_SRCS akonadi_serializer_bookmark.cpp)
kde4_add_plugin(akonadi_serializer_bookmark ${akonadi_serializer_bookmark_SRCS})
target_link_libraries(akonadi_serializer_bookmark ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} ${KDE4_AKONADI_LIBS})
install(TARGETS akonadi_serializer_bookmark DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install( FILES
  akonadi_serializer_addressee.desktop
  akonadi_serializer_mail.desktop
  akonadi_serializer_kcal.desktop
  akonadi_serializer_bookmark.desktop
DESTINATION ${DATA_INSTALL_DIR}/akonadi/plugins/serializer)

