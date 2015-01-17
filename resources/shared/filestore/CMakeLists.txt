project(akonadi-filestore)


add_definitions(-DTRANSLATION_DOMAIN=\"akonadi-filestore\")

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
generate_export_header(akonadi-filestore BASE_NAME akonadi-filestore)

target_link_libraries(akonadi-filestore
                      KF5::AkonadiCore
                      KF5::KIOCore
)

set_target_properties(akonadi-filestore PROPERTIES VERSION ${KDEPIMRUNTIME_LIB_VERSION} SOVERSION ${KDEPIMRUNTIME_LIB_SOVERSION} )

install(TARGETS akonadi-filestore ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

if (BUILD_TESTING)
  add_subdirectory( autotests )
endif ()

