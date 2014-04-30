
set(knotes-migrator_srcs
    main.cpp
    knotesmigrator.cpp
    notelockattribute.cpp
    knotesmigratorconfig.cpp
    notealarmattribute.cpp
    notedisplayattribute.cpp
    showfoldernotesattribute.cpp
    ${MIGRATION_AKONADI_SHARED_SOURCES}
)

kde4_add_kcfg_files(knotes-migrator_srcs knoteconfig.kcfgc)

macro(add_resource_iface _kcfgFile _ifaceName _className)
  kcfg_generate_dbus_interface(${kdepim-runtime_SOURCE_DIR}/resources/${_kcfgFile} ${_ifaceName})
  string(TOLOWER ${_className} _codeFile)
  qt4_add_dbus_interface(knotes-migrator_srcs
    ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml ${_codeFile} ${_className}
  )
endmacro()

add_resource_iface( maildir/maildirresource.kcfg org.kde.Akonadi.Maildir.Settings MaildirSettings )

kde4_add_executable(knotes-migrator ${knotes-migrator_srcs})

target_link_libraries(knotes-migrator
    ${KDEPIMLIBS_AKONADI_LIBS}
    ${KDEPIMLIBS_KABC_LIBS}
    ${KDEPIMLIBS_KCAL_LIBS}
    ${KDEPIMLIBS_KMIME_LIBS}
    ${KDE4_KDECORE_LIBS}
    ${QT_QTGUI_LIBRARY}
)

install(TARGETS knotes-migrator ${INSTALL_TARGETS_DEFAULT_ARGS})
#install(FILES knotes-migratorrc DESTINATION ${CONFIG_INSTALL_DIR})
