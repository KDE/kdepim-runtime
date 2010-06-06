include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( kdeaccountsresource_SRCS
  ${AKONADI_SINGLEFILERESOURCE_SHARED_SOURCES}
  kdeaccountsresource.cpp
)

install( FILES kdeaccountsresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_ui_files(kdeaccountsresource_SRCS ${AKONADI_SINGLEFILERESOURCE_SHARED_UI})
kde4_add_kcfg_files(kdeaccountsresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/kdeaccountsresource.kcfg org.kde.Akonadi.KDEAccounts.Settings)
qt4_add_dbus_adaptor(kdeaccountsresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.KDEAccounts.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_kdeaccounts_resource ${kdeaccountsresource_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_kdeaccounts_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_kdeaccounts_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.KDEAccounts")
  set_target_properties(akonadi_kdeaccounts_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi KDEAccounts Resource")
endif (Q_WS_MAC)

target_link_libraries(akonadi_kdeaccounts_resource ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDEPIMLIBS_KABC_LIBS} ${KDE4_KIO_LIBS})

install(TARGETS akonadi_kdeaccounts_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
