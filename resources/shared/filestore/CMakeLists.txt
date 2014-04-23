project(akonadi-filestore)
kde_enable_exceptions()

set(akonadi-filestore_SRCS
  abstractlocalstore.cpp
  collectioncreatejob.cpp
  collectiondeletejob.cpp
  collectionfetchjob.cpp
  collectionmodifyjob.cpp
  collectionmovejob.cpp
  entitycompactchangeattribute.cpp
  itemcreatejob.cpp
  itemdeletejob.cpp
  itemfetchjob.cpp
  itemmodifyjob.cpp
  itemmovejob.cpp
  job.cpp
  session.cpp
  sessionimpls.cpp
  storecompactjob.cpp
)

add_library(akonadi-filestore ${LIBRARY_TYPE} ${akonadi-filestore_SRCS} )

target_link_libraries(akonadi-filestore
                      KF5::AkonadiCore
                      KF5::KIOCore
)

set_target_properties(akonadi-filestore PROPERTIES VERSION ${KF5_VERSION} SOVERSION ${KF5_VERSION} )

install(TARGETS akonadi-filestore ${INSTALL_TARGETS_DEFAULT_ARGS})

if (KDE4_BUILD_TESTS)
  add_subdirectory( tests )
endif ()

