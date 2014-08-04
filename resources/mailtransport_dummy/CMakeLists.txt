set( mtdummyresource_SRCS
  configdialog.cpp
  mtdummyresource.cpp
)

install( FILES mtdummyresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

ki18n_wrap_ui(mtdummyresource_SRCS settings.ui)
kconfig_add_kcfg_files(mtdummyresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/mtdummyresource.kcfg org.kde.Akonadi.MailTransportDummy.Settings)
qt5_add_dbus_adaptor(mtdummyresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.MailTransportDummy.Settings.xml settings.h Settings
)

add_executable(akonadi_mailtransport_dummy_resource ${mtdummyresource_SRCS})

target_link_libraries(akonadi_mailtransport_dummy_resource KF5::AkonadiCore KF5::AkonadiWidgets KF5::AkonadiAgentBase)

install(TARGETS akonadi_mailtransport_dummy_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
