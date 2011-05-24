macro_ensure_version("4.6.40" "${KDEPIMLIBS_VERSION}" HAVE_TORAWSTRING)

configure_file(config-kcalcore.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kcalcore.h)

add_subdirectory( tests )

include_directories(
        ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

set(akonadi_serializer_addressee_PART_SRCS akonadi_serializer_addressee.cpp )
kde4_add_plugin(akonadi_serializer_addressee ${SERIALIZER_TYPE} ${akonadi_serializer_addressee_PART_SRCS})
target_link_libraries(akonadi_serializer_addressee ${KDE4_KDECORE_LIBS} ${KDEPIMLIBS_KABC_LIBS} ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_AKONADI_KABC_LIBS})
install(TARGETS akonadi_serializer_addressee DESTINATION ${PLUGIN_INSTALL_DIR})

set(akonadi_serializer_mail_PART_SRCS akonadi_serializer_mail.cpp )
kde4_add_plugin(akonadi_serializer_mail ${SERIALIZER_TYPE} ${akonadi_serializer_mail_PART_SRCS})
target_link_libraries(akonadi_serializer_mail ${KDE4_KDECORE_LIBS} ${KDEPIMLIBS_KMIME_LIBS} ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} ${AKONADI_COMMON_LIBRARIES} )
install(TARGETS akonadi_serializer_mail DESTINATION ${PLUGIN_INSTALL_DIR})

if(KDEPIMLIBS_KCAL_LIBRARY)
set(akonadi_serializer_kcal_SRCS akonadi_serializer_kcal.cpp)
kde4_add_plugin(akonadi_serializer_kcal ${SERIALIZER_TYPE} ${akonadi_serializer_kcal_SRCS})
target_link_libraries(akonadi_serializer_kcal ${KDE4_KDECORE_LIBS} ${KDEPIMLIBS_KCAL_LIBS} ${KDEPIMLIBS_AKONADI_LIBS})
install(TARGETS akonadi_serializer_kcal DESTINATION ${PLUGIN_INSTALL_DIR})
endif(KDEPIMLIBS_KCAL_LIBRARY)

set(akonadi_serializer_kcalcore_SRCS akonadi_serializer_kcalcore.cpp)
kde4_add_plugin(akonadi_serializer_kcalcore ${SERIALIZER_TYPE} ${akonadi_serializer_kcalcore_SRCS})
target_link_libraries(akonadi_serializer_kcalcore ${KDE4_KDECORE_LIBS} ${KDEPIMLIBS_KCALUTILS_LIBS} ${KDEPIMLIBS_KCALCORE_LIBS} ${KDEPIMLIBS_AKONADI_LIBS})
install(TARGETS akonadi_serializer_kcalcore DESTINATION ${PLUGIN_INSTALL_DIR})

if(NOT WINCE)

set(akonadi_serializer_bookmark_SRCS akonadi_serializer_bookmark.cpp)
kde4_add_plugin(akonadi_serializer_bookmark ${SERIALIZER_TYPE} ${akonadi_serializer_bookmark_SRCS})
target_link_libraries(akonadi_serializer_bookmark ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_AKONADI_LIBS})
install(TARGETS akonadi_serializer_bookmark DESTINATION ${PLUGIN_INSTALL_DIR})
endif(NOT WINCE)

set(akonadi_serializer_contactgroup_PART_SRCS akonadi_serializer_contactgroup.cpp )
kde4_add_plugin(akonadi_serializer_contactgroup ${SERIALIZER_TYPE} ${akonadi_serializer_contactgroup_PART_SRCS})
target_link_libraries(akonadi_serializer_contactgroup ${KDE4_KDECORE_LIBS} ${KDEPIMLIBS_KABC_LIBS} ${KDEPIMLIBS_AKONADI_LIBS}
                                                      ${KDEPIMLIBS_AKONADI_KABC_LIBS} ${KDEPIMLIBS_AKONADI_CONTACT_LIBS})
install(TARGETS akonadi_serializer_contactgroup DESTINATION ${PLUGIN_INSTALL_DIR})

if(NOT WINCE)
set( akonadi_serializer_microblog_SRCS akonadi_serializer_microblog.cpp )
kde4_add_plugin(akonadi_serializer_microblog ${SERIALIZER_TYPE} ${akonadi_serializer_microblog_SRCS})
target_link_libraries(akonadi_serializer_microblog ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTCORE_LIBRARY} ${KDE4_KDECORE_LIBS} ${QT_QTXML_LIBRARY} ${KDEPIMLIBS_KPIMUTILS_LIBS} ${KDEPIMLIBS_MICROBLOG_LIBS})
install(TARGETS akonadi_serializer_microblog DESTINATION ${PLUGIN_INSTALL_DIR})
endif(NOT WINCE)

########### install files ###############

install( FILES
  akonadi_serializer_addressee.desktop
  akonadi_serializer_mail.desktop
  akonadi_serializer_kcal.desktop
  akonadi_serializer_kcalcore.desktop
  akonadi_serializer_bookmark.desktop
  akonadi_serializer_contactgroup.desktop
  akonadi_serializer_microblog.desktop
DESTINATION ${DATA_INSTALL_DIR}/akonadi/plugins/serializer)

