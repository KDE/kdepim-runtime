include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
)

kde_enable_exceptions()

########### next target ###############

set( kdeaccountsresource_SRCS
  ${AKONADI_SINGLEFILERESOURCE_SHARED_SOURCES}
  kdeaccountsresource.cpp
)

install( FILES kdeaccountsresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

qt5_wrap_ui(kdeaccountsresource_SRCS ${AKONADI_SINGLEFILERESOURCE_SHARED_UI})
kconfig_add_kcfg_files(kdeaccountsresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/kdeaccountsresource.kcfg org.kde.Akonadi.KDEAccounts.Settings)
qt5_add_dbus_adaptor(kdeaccountsresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.KDEAccounts.Settings.xml settings.h Settings
)

add_executable(akonadi_kdeaccounts_resource ${kdeaccountsresource_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_kdeaccounts_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_kdeaccounts_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.KDEAccounts")
  set_target_properties(akonadi_kdeaccounts_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi KDEAccounts Resource")
endif ()

target_link_libraries(akonadi_kdeaccounts_resource KF5::AkonadiCore   ${KDE4_KDECORE_LIBS} ${KDEPIMLIBS_KABC_LIBS} KF5::KIOCore)

install(TARGETS akonadi_kdeaccounts_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
