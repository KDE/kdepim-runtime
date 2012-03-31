
set( akregatormigrator_SRCS
  main.cpp
  akregatormigrator.cpp
  itemimportreader.cpp
  importitemsjob.cpp
  itemsync.cpp
  ${MIGRATION_AKONADI_SHARED_SOURCES}
)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)

#macro(add_resource_iface _kcfgFile _ifaceName _className)
#  kcfg_generate_dbus_interface(${kdepim-runtime_SOURCE_DIR}/resources/${_kcfgFile} ${_ifaceName})
#  string(TOLOWER ${_className} _codeFile)
#  set_source_files_properties( ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml PROPERTIES INCLUDE "metatype.h")
#  qt4_add_dbus_interface(akregatormigrator_SRCS
#    ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml ${_codeFile} ${_className}
#  )
#endmacro(add_resource_iface)

#qt4_add_dbus_adaptor(krsslocalresource_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.RssLocal.Settings.xml settings.h Settings)

#add_resource_iface( maildir/maildirresource.kcfg
#                    org.kde.Akonadi.Maildir.Settings MaildirSettings )

qt4_add_dbus_interfaces(akregatormigrator_SRCS org.kde.Akonadi.RssLocal.Settings.xml)


kde4_add_executable( akregatormigrator ${akregatormigrator_SRCS} )

target_link_libraries(akregatormigrator
  ${QT_QTXML_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KDEPIMLIBS_LIBS}
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_SYNDICATION_LIBS}
  ${KDEPIMLIBS_KRSS_LIBS}
)

install(TARGETS akregatormigrator ${INSTALL_TARGETS_DEFAULT_ARGS})

