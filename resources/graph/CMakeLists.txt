# SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
# SPDX-License-Identifier: LGPL-2.0-or-later
#
# Akonadi resource for Microsoft 365 / Exchange Online via the Microsoft Graph API.
# Structure mirrors resources/ews. Developed at
# https://github.com/mzilinski/akonadi-microsoft365 (standalone build + live test tools).

add_subdirectory(graphclient)

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_graph_resource\")

ecm_qt_declare_logging_category(graph_common_SRCS
    HEADER graph_debug.h
    IDENTIFIER GRAPH_LOG
    CATEGORY_NAME org.kde.pim.graph
    DESCRIPTION "graph resource (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME
)

# ---- receiving resource ----------------------------------------------------
set(graphresource_SRCS
    graphresource.cpp
    graphsyncstateattribute.cpp
    mail/graphmailhandler.cpp
    calendar/grapheventhandler.cpp
    contact/graphcontacthandler.cpp
    todo/graphtodohandler.cpp
    jobs/graphbatchjob.cpp
    jobs/graphfetchfoldersjob.cpp
    jobs/graphfetchitemsjob.cpp
    jobs/graphfetchitempayloadjob.cpp
    jobs/graphfetchpimitemsjob.cpp
    jobs/graphmigrateidsjob.cpp
    graphresource.h
    graphsyncstateattribute.h
    mail/graphmailhandler.h
    calendar/grapheventhandler.h
    contact/graphcontacthandler.h
    todo/graphtodohandler.h
    jobs/graphbatchjob.h
    jobs/graphfetchfoldersjob.h
    jobs/graphfetchitemsjob.h
    jobs/graphfetchitempayloadjob.h
    jobs/graphfetchpimitemsjob.h
    jobs/graphmigrateidsjob.h
    ${graph_common_SRCS}
)

kconfig_add_kcfg_files(graphresource_SRCS graphsettingsbase.kcfgc)

# D-Bus interface: the MTA agent forwards outgoing MIME via sendMessage()/messageSent().
qt_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/graphresource.h
    org.kde.Akonadi.Graph.Resource.xml
    OPTIONS -a
)
qt_add_dbus_adaptor(graphresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Graph.Resource.xml
    graphresource.h GraphResource graphresourceadaptor GraphResourceAdaptor
)

add_executable(akonadi_graph_resource ${graphresource_SRCS})
target_include_directories(
    akonadi_graph_resource
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(
    akonadi_graph_resource
    KPim6::AkonadiAgentWidgetBase
    KPim6::AkonadiCore
    KPim6::AkonadiMime
    KF6::Mime
    KF6::CalendarCore
    KF6::Contacts
    KF6::I18n
    KF6::ConfigCore
    KF6::ConfigGui
    Qt::DBus
    graphclient
)

# ---- transport (send) resource ---------------------------------------------
# KMail requires separate send/receive resources.
set(graphmtaresource_SRCS
    graphmtaresource.cpp
    graphmtaresource.h
    ${graph_common_SRCS}
)
qt_add_dbus_interface(graphmtaresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Graph.Resource.xml
    graphresourceinterface
)
add_executable(akonadi_graphmta_resource ${graphmtaresource_SRCS})
target_include_directories(
    akonadi_graphmta_resource
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(
    akonadi_graphmta_resource
    KPim6::AkonadiAgentWidgetBase
    KPim6::AkonadiCore
    KF6::Mime
    KF6::I18n
    Qt::DBus
    graphclient
)

# ---- config plugin (loaded by the client's agent configuration dialog) ------
kcoreaddons_add_plugin(graphconfig
    INSTALL_NAMESPACE "pim6/akonadi/config"
)
target_sources(graphconfig PRIVATE graphconfig.cpp)
target_link_libraries(
    graphconfig
    PUBLIC
        KPim6::AkonadiCore
        KF6::ConfigCore
        KF6::I18n
        Qt::Widgets
)

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/graphresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
install(
    TARGETS
        akonadi_graph_resource
        ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/graphmtaresource.desktop
    DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents"
)
install(
    TARGETS
        akonadi_graphmta_resource
        ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)
