add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_imap_resource\")

########### next target ###############

set(kolabresource_SRCS
    kolabresource.cpp
    kolabresource.h
)

add_executable(akonadi_kolab_resource ${kolabresource_SRCS} ${kolabresource_common_SRCS})

target_link_libraries(akonadi_kolab_resource
    KPim6::AkonadiCore
    KPim6::AkonadiAgentBase
    KF6::I18n
    KF6::Notifications
    KF6::ConfigCore
)

install(FILES kolabresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
install(TARGETS akonadi_kolab_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.akonadi_kolab_resource.desktop DESTINATION ${KDE_INSTALL_APPDIR})

install(
    FILES akonadi_kolab_resource.notifyrc
    DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR}
)

kcoreaddons_add_plugin(kolabconfig
    INSTALL_NAMESPACE "pim6/akonadi/config"
)

ki18n_wrap_ui(kolabconfig ../imap/setupserverview_desktop.ui)

target_sources(kolabconfig PRIVATE
    kolabconfig.cpp
)

target_link_libraries(kolabconfig PUBLIC
    KPim6::AkonadiCore
    KPim6::AkonadiWidgets
    KPim6::AkonadiAgentWidgetBase
    KF6::I18n
)
