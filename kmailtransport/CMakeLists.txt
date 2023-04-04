kcoreaddons_add_plugin(mailtransport_akonadiplugin INSTALL_NAMESPACE "pim6/mailtransport")
target_sources(mailtransport_akonadiplugin PRIVATE
    akonadimailtransportplugin.cpp
    resourcesendjob.cpp
    resourcesendjob_p.h
    akonadimailtransportplugin.h
    )

ecm_qt_declare_logging_category(mailtransport_akonadiplugin HEADER mailtransportplugin_akonadi_debug.h IDENTIFIER
        MAILTRANSPORT_AKONADI_LOG CATEGORY_NAME org.kde.pim.mailtransport.akonadiplugin
        DESCRIPTION "kmailtransport (akonadi plugin)"
        EXPORT KDEPIMRUNTIME
    )




target_link_libraries(mailtransport_akonadiplugin
    KPim6::AkonadiMime
    KPim6::Mime
    KPim6::MailTransport
    KF6::CoreAddons
    KPim6::AkonadiCore
    KPim6::AkonadiWidgets
    KF6::I18n
    KF6::ConfigWidgets
    Qt::DBus
    )
