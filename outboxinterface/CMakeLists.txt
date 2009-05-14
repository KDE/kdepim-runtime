find_package(KDE4 REQUIRED)
find_package(KdepimLibs REQUIRED)
find_package(Akonadi REQUIRED)
include(KDE4Defaults)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES} ${KDEPIMLIBS_INCLUDE_DIRS})

# TODO: 5324 is mailtransport. we need one of our own!
add_definitions( -DKDE_DEFAULT_DEBUG_AREA=5324 )

set(outboxinterface_lib_srcs
  localfolders.cpp
)

kde4_add_library(outboxinterface SHARED ${outboxinterface_lib_srcs})
target_link_libraries(outboxinterface ${KDE4_AKONADI_LIBS} ${KDE4_KIO_LIBS})
set_target_properties(outboxinterface PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )

install(TARGETS outboxinterface ${INSTALL_TARGETS_DEFAULT_ARGS})

install( FILES 
        outboxinterface_export.h
        localfolders.h
        DESTINATION ${INCLUDE_INSTALL_DIR}/outboxinterface COMPONENT Devel)

