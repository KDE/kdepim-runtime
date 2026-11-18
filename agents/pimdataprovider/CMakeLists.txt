add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_pimdataprovider_agent\")

kde_enable_exceptions()

add_executable(akonadi_pimdataprovider_agent pimdataprovider_types.h)

ecm_qt_declare_logging_category(pimdataprovider_common_SRCS HEADER pimdataprovider_debug.h IDENTIFIER PIMDATAPROVIDER_LOG CATEGORY_NAME org.kde.pim.pimdataprovider
        DESCRIPTION "pimdataprovider agent (kdepim-runtime)"
        OLD_CATEGORY_NAMES log_pimdataprovider
        EXPORT KDEPIMRUNTIME
)

qt_add_dbus_adaptor(pimdataprovideragent_SRCS
        interfaces/org.pim.dataprovider.Calendar.Management.xml
        pimdataprovideragent.h
        Akonadi::PimDataProviderAgent
)

qt_add_dbus_adaptor(pimdataprovideragent_SRCS
        interfaces/org.pim.dataprovider.Calendar.Server.xml
        pimdataproviderserver.h
        Akonadi::PimDataProviderServer
)

qt_add_dbus_adaptor(pimdataprovideragent_SRCS
        interfaces/org.pim.dataprovider.Calendar.Server.Calendar.xml
        pimdataprovidercalendar.h
        Akonadi::PimDataProviderCalendar
)

target_sources(
    akonadi_pimdataprovider_agent
    PRIVATE
        pimdataprovideragent.cpp
        pimdataprovideragent.h
        pimdataproviderserver.cpp
        pimdataproviderserver.h
        pimdataprovidercalendar.cpp
        pimdataprovidercalendar.h
        ${pimdataprovideragent_SRCS}
        ${pimdataprovider_common_SRCS}
)

target_link_libraries(
    akonadi_pimdataprovider_agent
    KPim6::AkonadiAgentBase
    KPim6::AkonadiAgentWidgetBase
    KPim6::AkonadiCore
    KPim6::AkonadiCalendar
    KF6::CalendarCore
    KF6::ConfigCore
    Qt6::DBus
)

install(
    TARGETS
        akonadi_pimdataprovider_agent
        ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)
install(FILES pimdataprovider.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
