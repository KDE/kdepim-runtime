add_definitions(-DTRANSLATION_DOMAIN=\"kcm_ldap\")

add_library(kcm_ldap MODULE)
target_sources( kcm_ldap PRIVATE
    kcmldap.cpp
    kcmldap_p.h
    )


target_link_libraries(kcm_ldap KF5::KCMUtils KF5::I18n KF5::Ldap)

kcoreaddons_desktop_to_json(kcm_ldap kcmldap.desktop DEFAULT_SERVICE_TYPE)

install(TARGETS kcm_ldap DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim/kcms/kaddressbook)

install(FILES kcmldap.desktop DESTINATION ${KDE_INSTALL_KSERVICESDIR})

