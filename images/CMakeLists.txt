foreach(_resource "apple" "citadel" "davical" "egroupware" "fastmail" "kopano" "nextcloud" "sogo" "yahoo" "zimbra" "zoho")
    ecm_install_icons(ICONS 128-apps-account-${_resource}.png  16-apps-account-${_resource}.png  32-apps-account-${_resource}.png 48-apps-account-${_resource}.png  64-apps-account-${_resource}.png DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor)
endforeach()
