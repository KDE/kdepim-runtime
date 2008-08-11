set(kres-migrator_srcs
    main.cpp
    kabcmigrator.cpp
)

kcfg_generate_dbus_interface(${CMAKE_SOURCE_DIR}/akonadi/resources/vcard/vcardresource.kcfg org.kde.Akonadi.VCard.Settings)
qt4_add_dbus_interface(kres-migrator_srcs
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.VCard.Settings.xml vcardsettings VCardSettings
)

kde4_add_app_icon(kres-migrator_srcs "${KDE4_ICON_DIR}/oxygen/*/apps/email.png")
kde4_add_executable(kres-migrator ${kres-migrator_srcs})
target_link_libraries(kres-migrator ${KDE4_AKONADI_LIBS} ${KDE4_KRESOURCE_LIBS} ${KDE4_KABC_LIBS})
install(TARGETS kres-migrator ${INSTALL_TARGETS_DEFAULT_ARGS})
