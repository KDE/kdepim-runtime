set(kmail-migrator_srcs
  kmailmigration_debug.cpp 
  abstractcollectionmigrator.cpp
  emptyresourcecleaner.cpp
  imapcacheadapter.cpp
  imapcachecollectionmigrator.cpp
  imapcachelocalimporter.cpp
  kmailmigrator.cpp
  localfolderscollectionmigrator.cpp
  main.cpp
  ${MIGRATION_AKONADI_SHARED_SOURCES}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/mixedmaildir/mixedmaildirstore.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/mixedmaildir/mixedmaildir_debug.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/mixedmaildir/mixedmaildirresource_debug.cpp
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/maildir
  ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/mbox
  ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/mixedmaildir
  ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/shared
  ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/shared/filestore
)

macro(add_resource_iface _kcfgFile _ifaceName _className)
  kcfg_generate_dbus_interface(${kdepim-runtime_SOURCE_DIR}/resources/${_kcfgFile} ${_ifaceName})
  string(TOLOWER ${_className} _codeFile)
  set_source_files_properties( ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml PROPERTIES INCLUDE "metatype.h")
  qt5_add_dbus_interface(kmail-migrator_srcs
    ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml ${_codeFile} ${_className}
  )
endmacro()

add_resource_iface( imap/imapresource.kcfg
                    org.kde.Akonadi.Imap.Settings ImapSettings )
add_resource_iface( pop3/settings.kcfg
                    org.kde.Akonadi.POP3.Settings Pop3Settings )
add_resource_iface( mbox/mboxresource.kcfg
                    org.kde.Akonadi.Mbox.Settings MboxSettings )
add_resource_iface( maildir/maildirresource.kcfg
                    org.kde.Akonadi.Maildir.Settings MaildirSettings )
add_resource_iface( mixedmaildir/mixedmaildirresource.kcfg
                    org.kde.Akonadi.MixedMaildir.Settings MixedMaildirSettings )

#file(GLOB ICONS_SRCS "${KDE4_ICON_DIR}/oxygen/*/apps/akonadi.png")
#ecm_add_app_icon(kmail-migrator_srcs ICONS ${ICONS_SRCS})
add_executable(kmail-migrator ${kmail-migrator_srcs} )
target_link_libraries(kmail-migrator
  KF5::Mbox
  KF5::Mime
  KF5::AkonadiMime
  KF5::AkonadiCore
  KF5::AkonadiAgentBase  
  KF5::KIOCore
  KF5::MailTransport
  akonadi-filestore
  maildir
  kmindexreader
  akonadi-singlefileresource
)

install(TARGETS kmail-migrator ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES kmail-migratorrc DESTINATION ${KDE_INSTALL_CONFDIR})
