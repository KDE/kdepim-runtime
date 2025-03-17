
add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_singlefile_resource\")

add_library(akonadi-singlefileresource
    collectionflagsattribute.cpp
    createandsettagsjob.cpp
    imapquotaattribute.cpp
    singlefileresourcebase.cpp

    collectionflagsattribute.h
    createandsettagsjob.h
    imapquotaattribute.h
    singlefileresourcebase.h
)

generate_export_header(akonadi-singlefileresource BASE_NAME akonadi-singlefileresource)
if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi-singlefileresource PROPERTIES UNITY_BUILD ON)
endif()

target_link_libraries(akonadi-singlefileresource
PUBLIC
    KF6::I18n
    KPim6::AkonadiAgentBase
PRIVATE
    KPim6::AkonadiCore
    KF6::KIOCore
    KPim6::IMAP
)

set_target_properties(akonadi-singlefileresource PROPERTIES
    VERSION ${KDEPIMRUNTIME_LIB_VERSION}
    SOVERSION ${KDEPIMRUNTIME_LIB_SOVERSION}
)
install(TARGETS akonadi-singlefileresource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

# Widget

set(AKONADI_SINGLEFILERESOURCE_SHARED_UI
    singlefileresourceconfigwidget_desktop.ui
    singlefileresourceconfigwidget.ui
)

ki18n_wrap_ui(akonadi-singlefileresource-widget_SRCS ${AKONADI_SINGLEFILERESOURCE_SHARED_UI} settingsdialog.ui)

add_library(akonadi-singlefileresource-widget
    ${akonadi-singlefileresource-widget_SRCS}
    singlefileresourceconfigwidgetbase.cpp
    singlefileresourceconfigwidgetbase.h
)

generate_export_header(akonadi-singlefileresource-widget BASE_NAME akonadi-singlefileresource-widget)

set_target_properties(akonadi-singlefileresource-widget PROPERTIES
    VERSION ${KDEPIMRUNTIME_LIB_VERSION}
    SOVERSION ${KDEPIMRUNTIME_LIB_SOVERSION}
)

target_link_libraries(akonadi-singlefileresource-widget
PUBLIC
    akonadi-singlefileresource
    KF6::Completion
    KF6::KIOWidgets
    KF6::ConfigWidgets
    KF6::WidgetsAddons
PRIVATE
    KPim6::AkonadiCore
    KF6::KIOCore
    KPim6::IMAP
)


if(HAVE_ACTIVITY_SUPPORT)
    target_link_libraries(akonadi-singlefileresource-widget
        PUBLIC
        KPim6::PimCommonActivities
    )
endif()

install(TARGETS akonadi-singlefileresource-widget ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
