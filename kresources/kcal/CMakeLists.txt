set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

include_directories (${Boost_INCLUDE_DIR})

########### next target ###############

set(kcal_akonadi_SRCS resourceakonadiplugin.cpp resourceakonadi.cpp resourceakonadi_p.cpp resourceakonadiconfig.cpp subresource.cpp ${RESOURCE_AKONADI_SHARED_SOURCES})


kde4_add_plugin(kcal_akonadi ${kcal_akonadi_SRCS})

target_link_libraries(kcal_akonadi ${KDE4_KCAL_LIBS} ${KDE4_AKONADI_LIBS} akonadi-kcal ${KDE4_KUTILS_LIBS})


install(TARGETS kcal_akonadi  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES akonadi.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kresources/kcal )
