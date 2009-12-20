project(akonadi-filestore)

find_package(KDE4 REQUIRED)
find_package(Akonadi REQUIRED)
find_package(KdepimLibs REQUIRED)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../..
  ${CMAKE_CURRENT_SOURCE_DIR}/libmbox
  ${AKONADI_INCLUDE_DIR}
  ${KDE4_INCLUDES}
  ${KDEPIMLIBS_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${Boost_INCLUDE_DIR}
)

add_subdirectory( libmbox )

include(KDE4Defaults)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

set(akonadi-filestore_SRCS
  collectionfetchjob.cpp
  directaccessstore.cpp
  itemcreatejob.cpp
  itemdeletejob.cpp
  itemfetchjob.cpp
  itemmodifyjob.cpp
  job.cpp
  session.cpp
  sessionimpls.cpp
  storecompactjob.cpp
  augmentedmbox/augmentedmboxstore.cpp
  mappedvcard/mappedvcardstore.cpp
)

kde4_add_library(akonadi-filestore SHARED ${akonadi-filestore_SRCS} )

target_link_libraries(akonadi-filestore
                      mbox
                      ${AKONADI_COMMON_LIBRARIES}
                      ${QT_QTCORE_LIBRARY}
                      ${QT_QTGUI_LIBRARY}
                      ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
                      ${KDEPIMLIBS_AKONADI_LIBS}
                      ${KDE4_KIO_LIBS}
                      ${KDEPIMLIBS_KABC_LIBS}
                      ${KDEPIMLIBS_KMIME_LIBS}
)

set_target_properties(akonadi-filestore PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )

install(TARGETS akonadi-filestore ${INSTALL_TARGETS_DEFAULT_ARGS})

if (KDE4_BUILD_TESTS)
  add_subdirectory( tests )
endif (KDE4_BUILD_TESTS)

