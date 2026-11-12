add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_pimdataprovider_agent\")

kde_enable_exceptions()

add_executable(akonadi_pimdataprovider_agent pimdataprovider_types.h)

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
)

target_link_libraries(
    akonadi_pimdataprovider_agent
    KPim6::AkonadiAgentBase
    KPim6::AkonadiAgentWidgetBase
    KPim6::AkonadiCore
    KPim6::AkonadiCalendar
    KF6::CalendarCore
    Qt6::DBus
)

install(
    TARGETS
        akonadi_pimdataprovider_agent
        ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)
install(FILES pimdataprovider.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
