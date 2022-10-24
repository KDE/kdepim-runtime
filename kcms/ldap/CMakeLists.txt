add_definitions(-DTRANSLATION_DOMAIN=\"kcm_ldap\")

add_library(kcm_ldap MODULE)
target_sources( kcm_ldap PRIVATE
    kcmldap.cpp
    kcmldap.h
    )


target_link_libraries(kcm_ldap KF5::CoreAddons KF5::KCMUtils KF5::I18n KF5::Ldap)

install(TARGETS kcm_ldap DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim${QT_MAJOR_VERSION}/kcms/kaddressbook)

