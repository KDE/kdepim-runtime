
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

ecm_qt_declare_logging_category(akonadi-filestore_SRCS HEADER akonadifilestore_debug.h IDENTIFIER AKONADIFILESTORE_LOG CATEGORY_NAME org.kde.pim.filestore
    DESCRIPTION "resource filestore lib (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME
    )


add_library(akonadi-filestore  ${akonadi-filestore_SRCS} )
generate_export_header(akonadi-filestore BASE_NAME akonadi-filestore)

target_link_libraries(akonadi-filestore
    PUBLIC
    KF5::AkonadiCore
    PRIVATE
    KF5::I18n
    )

set_target_properties(akonadi-filestore PROPERTIES VERSION ${KDEPIMRUNTIME_LIB_VERSION} SOVERSION ${KDEPIMRUNTIME_LIB_SOVERSION} )

install(TARGETS akonadi-filestore ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

if (BUILD_TESTING)
    add_subdirectory( autotests )
endif ()

