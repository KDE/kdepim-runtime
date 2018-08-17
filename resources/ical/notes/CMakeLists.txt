include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared
    ${kdepim-runtime_SOURCE_DIR}
)

set(notesresource_common_SRCS)
kconfig_add_kcfg_files(notesresource_common_SRCS settings.kcfgc)

############################## Resource #################################

add_definitions( -DSETTINGS_NAMESPACE=Akonadi_Aknotes_Resource )

set( notesresource_SRCS
  ../shared/icalresourcebase.cpp
  ../shared/icalresource.cpp
  notesresource.cpp
  ${notesresource_common_SRCS}
)

install( FILES notesresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/notesresource.kcfg org.kde.Akonadi.Notes.Settings)
qt5_add_dbus_adaptor(notesresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Notes.Settings.xml settings.h Akonadi_Aknotes_Resource::Settings icalsettingsadaptor ICalSettingsAdaptor
)

add_executable(akonadi_notes_resource ${notesresource_SRCS})

target_link_libraries(akonadi_notes_resource KF5::AkonadiCore  KF5::KIOCore KF5::CalendarCore KF5::AkonadiAgentBase KF5::DBusAddons akonadi-singlefileresource)

install(TARGETS akonadi_notes_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


############################ Config plugin ##################################

kcoreaddons_add_plugin(notesconfig
    SOURCES notesconfig.cpp ${notesresource_common_SRCS}
    JSON "notesconfig.json"
    INSTALL_NAMESPACE "akonadi/config"
)

target_link_libraries(notesconfig
    KF5::AkonadiCore
    akonadi-singlefileresource
)
