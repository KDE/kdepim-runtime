include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared
    ${kdepim-runtime_SOURCE_DIR}
    ${Boost_INCLUDE_DIR}
)




########### next target ###############

add_definitions( -DSETTINGS_NAMESPACE=Akonadi_Aknotes_Resource )

set( notesresource_SRCS
  ../shared/icalresourcebase.cpp
  ../shared/icalresource.cpp
  notesresource.cpp
)

install( FILES notesresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kconfig_add_kcfg_files(notesresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/notesresource.kcfg org.kde.Akonadi.Notes.Settings)
qt5_add_dbus_adaptor(notesresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Notes.Settings.xml settings.h Akonadi_Aknotes_Resource::Settings icalsettingsadaptor ICalSettingsAdaptor
)

add_executable(akonadi_notes_resource ${notesresource_SRCS})

target_link_libraries(akonadi_notes_resource KF5::AkonadiCore  KF5::KIOCore KF5::CalendarCore KF5::AkonadiAgentBase akonadi-singlefileresource)

install(TARGETS akonadi_notes_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
