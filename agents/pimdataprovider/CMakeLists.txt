add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_pimdataprovider_agent\")

kde_enable_exceptions()

add_executable(akonadi_pimdataprovider_agent)

target_sources(
    akonadi_pimdataprovider_agent
    PRIVATE
        pimdataprovideragent.cpp
        pimdataprovideragent.h
)

if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(
        akonadi_pimdataprovider_agent
        PROPERTIES
            UNITY_BUILD
                ON
    )
endif()

if(APPLE)
    embed_info_plist(
        akonadi_pimdataprovider_agent
        "org.kde.Akonadi.PimDataProvider"
        "KDE Akonadi PimDataProvider Agent"
    )
endif()

target_link_libraries(
    akonadi_pimdataprovider_agent
    KPim6::AkonadiAgentBase
    KPim6::AkonadiCore
)

install(
    TARGETS
        akonadi_pimdataprovider_agent
        ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)
install(FILES pimdataprovider.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
