
project(akonadi-singlefileresource)


add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_singlefile_resource\")

set( AKONADI_SINGLEFILERESOURCE_SHARED_SOURCES
  singlefileresourcebase.cpp
  singlefileresourceconfigdialogbase.cpp
)

if (KDEPIM_MOBILE_UI)
set( AKONADI_SINGLEFILERESOURCE_SHARED_UI
  singlefileresourceconfigdialog_mobile.ui
  singlefileresourceconfigdialog.ui
)
else ()
set( AKONADI_SINGLEFILERESOURCE_SHARED_UI
  singlefileresourceconfigdialog_desktop.ui
  singlefileresourceconfigdialog.ui
)
endif ()

set( AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES
  collectionannotationsattribute.cpp
  collectionflagsattribute.cpp
)

set( AKONADI_IMAPATTRIBUTES_SHARED_SOURCES
  imapaclattribute.cpp
  imapquotaattribute.cpp
)


set(akonadi-singlefileresource_SRCS
  ${AKONADI_SINGLEFILERESOURCE_SHARED_SOURCES}
  ${AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES}
  ${AKONADI_IMAPATTRIBUTES_SHARED_SOURCES}
  createandsettagsjob.cpp
)

ki18n_wrap_ui(akonadi-singlefileresource_SRCS ${AKONADI_SINGLEFILERESOURCE_SHARED_UI} settingsdialog.ui)

add_library(akonadi-singlefileresource ${LIBRARY_TYPE} ${akonadi-singlefileresource_SRCS} )
generate_export_header(akonadi-singlefileresource BASE_NAME akonadi-singlefileresource)

target_link_libraries(akonadi-singlefileresource
                      KF5::AkonadiCore
                      KF5::AkonadiAgentBase
                      KF5::KIOCore
                      KF5::Imap
)

set_target_properties(akonadi-singlefileresource PROPERTIES VERSION ${KDEPIMRUNTIME_LIB_VERSION} SOVERSION ${KDEPIMRUNTIME_LIB_SOVERSION} )
install(TARGETS akonadi-singlefileresource ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(autotests)
