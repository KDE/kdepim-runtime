find_package(KDE4 REQUIRED)
find_package(KdepimLibs REQUIRED)
find_package(Akonadi REQUIRED)

set(Boost_MINIMUM_VERSION "1.33.1")
find_package(Boost)
macro_log_feature(Boost_FOUND "boost" "Boost C++ Libraries" "http://www.boost.org" TRUE "" "Required by several critical KDEPIM apps.")

include(KDE4Defaults)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/.. ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES} ${KDEPIMLIBS_INCLUDE_DIRS})

# TODO: 5324 is mailtransport. we need one of our own!
add_definitions( -DKDE_DEFAULT_DEBUG_AREA=5324 )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

add_subdirectory( tests )

set(outboxinterface_lib_srcs
  localfolders.cpp
  messagequeuejob.cpp

  addressattribute.cpp
  dispatchmodeattribute.cpp
  errorattribute.cpp
  sentcollectionattribute.cpp
  transportattribute.cpp

  ../akonaditest/resourcetester/resourcesynchronizationjob.cpp
)

kde4_add_kcfg_files(outboxinterface_lib_srcs settings.kcfgc)
install(FILES outboxinterface.kcfg DESTINATION ${KCFG_INSTALL_DIR})

kde4_add_library(outboxinterface SHARED ${outboxinterface_lib_srcs})
target_link_libraries(outboxinterface ${KDE4_AKONADI_LIBS} ${KDE4_KIO_LIBS} ${KDE4_MAILTRANSPORT_LIBS} ${KDE4_KMIME_LIBS} )
set_target_properties(outboxinterface PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )


install(TARGETS outboxinterface ${INSTALL_TARGETS_DEFAULT_ARGS})

install( FILES 
        outboxinterface_export.h
        localfolders.h
        messagequeuejob.h

        addressattribute.h
        dispatchmodeattribute.h
        errorattribute.h
        sentcollectionattribute.h
        transportattribute.h

        DESTINATION ${INCLUDE_INSTALL_DIR}/outboxinterface COMPONENT Devel)

