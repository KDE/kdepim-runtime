include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared
    ${kdepim-runtime_SOURCE_DIR}
)

set(notesresource_common_SRCS)
kconfig_add_kcfg_files(notesresource_common_SRCS settings.kcfgc)

############################## Resource #################################

add_definitions(-DSETTINGS_NAMESPACE=Akonadi_Aknotes_Resource)
add_executable(akonadi_notes_resource)


install(FILES notesresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/notesresource.kcfg org.kde.Akonadi.Notes.Settings)
qt_add_dbus_adaptor(notesresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Notes.Settings.xml settings.h Akonadi_Aknotes_Resource::Settings icalsettingsadaptor ICalSettingsAdaptor
)
target_sources(akonadi_notes_resource PRIVATE
  ../shared/icalresourcebase.cpp
  ../shared/icalresource.cpp
  notesresource.cpp
  ../shared/icalresourcebase.h
  ../shared/icalresource.h
  notesresource.h
  ${notesresource_common_SRCS}
  ${notesresource_SRCS}
)


target_link_libraries(akonadi_notes_resource KPim6::AkonadiCore  KF6::KIOCore KF6::CalendarCore KPim6::AkonadiAgentBase akonadi-singlefileresource)

install(TARGETS akonadi_notes_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


############################ Config plugin ##################################

kcoreaddons_add_plugin(notesconfig
    INSTALL_NAMESPACE "pim6/akonadi/config"
)
target_sources(notesconfig PRIVATE notesconfig.cpp ${notesresource_common_SRCS})

target_link_libraries(notesconfig
    KPim6::AkonadiCore
    akonadi-singlefileresource
)
