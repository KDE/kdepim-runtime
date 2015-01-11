
set( kjotsmigrator_SRCS
  main.cpp
  kjotsmigrator.cpp
  ${MIGRATION_AKONADI_SHARED_SOURCES}
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/maildir
)

macro(add_resource_iface _kcfgFile _ifaceName _className)
  kcfg_generate_dbus_interface(${kdepim-runtime_SOURCE_DIR}/resources/${_kcfgFile} ${_ifaceName})
  string(TOLOWER ${_className} _codeFile)
  set_source_files_properties( ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml PROPERTIES INCLUDE "metatype.h")
  qt5_add_dbus_interface(kjotsmigrator_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml ${_codeFile} ${_className}
  )
endmacro()

add_resource_iface( maildir/maildirresource.kcfg
                    org.kde.Akonadi.Maildir.Settings MaildirSettings )

add_executable( kjotsmigrator ${kjotsmigrator_SRCS} )

target_link_libraries(kjotsmigrator
  KF5::AkonadiCore
  KF5::PimTextEdit
  KF5::Mime
  maildir
)

install(TARGETS kjotsmigrator ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
