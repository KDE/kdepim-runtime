add_definitions(-DTRANSLATION_DOMAIN=\"kcm_ldap\")

set( kcm_ldap_SRCS
    kcmldap.cpp
    )

add_library(kcm_ldap MODULE ${kcm_ldap_SRCS})


target_link_libraries(kcm_ldap KF5::KCMUtils KF5::I18n KF5::Ldap)

kcoreaddons_desktop_to_json(kcm_ldap kcmldap.desktop DEFAULT_SERVICE_TYPE)

install(TARGETS kcm_ldap DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES kcmldap.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

