add_definitions(-DTRANSLATION_DOMAIN=\"kcm_ldap\")

add_library(kcm_ldap MODULE)
target_sources(kcm_ldap PRIVATE
    kcmldap.cpp
    kcmldap.h
)


target_link_libraries(kcm_ldap KF6::CoreAddons KF6::KCMUtils KF6::I18n KPim6::LdapWidgets)

install(TARGETS kcm_ldap DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim6/kcms/common)

