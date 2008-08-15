set(kres-migrator_srcs
    main.cpp
    kabcmigrator.cpp
    kcalmigrator.cpp
    kresmigratorbase.cpp
    infodialog.cpp
)

kcfg_generate_dbus_interface(${CMAKE_SOURCE_DIR}/akonadi/resources/vcard/vcardresource.kcfg org.kde.Akonadi.VCard.Settings)
kcfg_generate_dbus_interface(${CMAKE_SOURCE_DIR}/akonadi/resources/ical/icalresource.kcfg org.kde.Akonadi.ICal.Settings)

qt4_add_dbus_interface(kres-migrator_srcs
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.VCard.Settings.xml vcardsettings VCardSettings
)
qt4_add_dbus_interface(kres-migrator_srcs
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.ICal.Settings.xml icalsettings ICalSettings
)

kde4_add_app_icon(kres-migrator_srcs "${KDE4_ICON_DIR}/oxygen/*/apps/akonadi.png")
kde4_add_executable(kres-migrator ${kres-migrator_srcs})
target_link_libraries(kres-migrator ${KDE4_AKONADI_LIBS} ${KDE4_KRESOURCE_LIBS} ${KDE4_KABC_LIBS} ${KDE4_KCAL_LIBS})
install(TARGETS kres-migrator ${INSTALL_TARGETS_DEFAULT_ARGS})
