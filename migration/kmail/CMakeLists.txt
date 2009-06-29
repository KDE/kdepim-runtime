set(kmail-migrator_srcs
  main.cpp
  kmailmigrator.cpp
  ${MIGRATION_AKONADI_SHARED_SOURCES}
)

macro(add_resource_iface _kcfgFile _ifaceName _className)
  kcfg_generate_dbus_interface(${kdepim-runtime_SOURCE_DIR}/resources/${_kcfgFile} ${_ifaceName})
  string(TOLOWER ${_className} _codeFile)
  qt4_add_dbus_interface(kmail-migrator_srcs
    ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml ${_codeFile} ${_className}
  )
endmacro(add_resource_iface)

add_resource_iface( imap/imapresource.kcfg
                    org.kde.Akonadi.Imap.Settings ImapSettings )
#add_resource_iface( pop3/pop3resource.kcfg
#                    org.kde.Akonadi.Pop3.Settings Pop3Settings )
add_resource_iface( mbox/mboxresource.kcfg
                    org.kde.Akonadi.Mbox.Settings MboxSettings )
add_resource_iface( maildir/maildirresource.kcfg
                    org.kde.Akonadi.Maildir.Settings MaildirSettings )

kde4_add_app_icon(kmail-migrator_srcs "${KDE4_ICON_DIR}/oxygen/*/apps/akonadi.png")
kde4_add_executable(kmail-migrator ${kmail-migrator_srcs})
target_link_libraries(kmail-migrator
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDE4_KDEUI_LIBS}
)

install(TARGETS kmail-migrator ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES kmail-migratorrc DESTINATION ${CONFIG_INSTALL_DIR})
