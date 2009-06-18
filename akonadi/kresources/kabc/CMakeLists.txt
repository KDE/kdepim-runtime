
#include_directories( ${CMAKE_SOURCE_DIR}/kabc )
include_directories( ${Boost_INCLUDE_DIR} )

########### next target ###############

set(kabc_akonadi_SRCS resourceakonadiplugin.cpp resourceakonadi.cpp resourceakonadi_p.cpp resourceakonadiconfig.cpp subresource.cpp
${RESOURCE_AKONADI_SHARED_SOURCES} )


kde4_add_plugin(kabc_akonadi ${kabc_akonadi_SRCS})

target_link_libraries(kabc_akonadi ${KDEPIMLIBS_KABC_LIBS} ${KDEPIMLIBS_AKONADI_LIBS} ${KDE4_KUTILS_LIBS})


install(TARGETS kabc_akonadi  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES akonadi.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kresources/kabc )
