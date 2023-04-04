
add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_singlefile_resource\")

set( AKONADI_SINGLEFILERESOURCE_SHARED_SOURCES
  singlefileresourcebase.cpp singlefileresourcebase.h
  singlefileresourceconfigwidgetbase.cpp singlefileresourceconfigwidgetbase.h
)

set( AKONADI_SINGLEFILERESOURCE_SHARED_UI
  singlefileresourceconfigwidget_desktop.ui
  singlefileresourceconfigwidget.ui
)

set( AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES
  collectionannotationsattribute.cpp
  collectionflagsattribute.cpp
  collectionannotationsattribute.h
  collectionflagsattribute.h
)

set( AKONADI_IMAPATTRIBUTES_SHARED_SOURCES
  imapaclattribute.cpp
  imapquotaattribute.cpp
  imapaclattribute.h
  imapquotaattribute.h
)


set(akonadi-singlefileresource_SRCS
  ${AKONADI_SINGLEFILERESOURCE_SHARED_SOURCES}
  ${AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES}
  ${AKONADI_IMAPATTRIBUTES_SHARED_SOURCES}
  createandsettagsjob.cpp createandsettagsjob.h
)

ki18n_wrap_ui(akonadi-singlefileresource_SRCS ${AKONADI_SINGLEFILERESOURCE_SHARED_UI} settingsdialog.ui)

add_library(akonadi-singlefileresource  ${akonadi-singlefileresource_SRCS} )
generate_export_header(akonadi-singlefileresource BASE_NAME akonadi-singlefileresource)
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi-singlefileresource PROPERTIES UNITY_BUILD ON)
endif()

target_link_libraries(akonadi-singlefileresource
PUBLIC
    KF6::I18n
    KF6::Completion
    KF6::KIOWidgets
    KF6::ConfigWidgets
PRIVATE
    KPim6::AkonadiCore
    KPim6::AkonadiAgentBase
    KF6::KIOCore
    KPim6::IMAP
)

set_target_properties(akonadi-singlefileresource PROPERTIES VERSION ${KDEPIMRUNTIME_LIB_VERSION} SOVERSION ${KDEPIMRUNTIME_LIB_SOVERSION} )
install(TARGETS akonadi-singlefileresource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

if (BUILD_TESTING)
   add_subdirectory(autotests)
endif()
