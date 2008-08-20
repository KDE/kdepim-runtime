
#include_directories( ${CMAKE_SOURCE_DIR}/kabc )

########### next target ###############

set(kabc_akonadi_SRCS resourceakonadiplugin.cpp resourceakonadi.cpp resourceakonadiconfig.cpp)


kde4_add_plugin(kabc_akonadi ${kabc_akonadi_SRCS})

target_link_libraries(kabc_akonadi ${KDE4_KABC_LIBS} ${KDE4_AKONADI_LIBS} ${KDE4_KUTILS_LIBS})


install(TARGETS kabc_akonadi  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES akonadi.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kresources/kabc )
