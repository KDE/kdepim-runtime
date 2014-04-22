include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

kde_enable_exceptions()

add_subdirectory(wizard)

########### next target ###############

set( mboxresource_SRCS
  ${AKONADI_SINGLEFILERESOURCE_SHARED_SOURCES}
  compactpage.cpp
  lockmethodpage.cpp
  deleteditemsattribute.cpp
  mboxresource.cpp
)

install( FILES mboxresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

qt5_wrap_ui(mboxresource_SRCS
  ${AKONADI_SINGLEFILERESOURCE_SHARED_UI}
  compactpage.ui
  lockfilepage.ui
)
kconfig_add_kcfg_files(mboxresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/mboxresource.kcfg org.kde.Akonadi.Mbox.Settings)
qt5_add_dbus_adaptor(mboxresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Mbox.Settings.xml settings.h Settings
)

add_library(akonadi_mbox_resource MODULE ${mboxresource_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_mbox_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_mbox_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.Mbox")
  set_target_properties(akonadi_mbox_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Mbox Resource")
endif ()

target_link_libraries(akonadi_mbox_resource ${KDEPIMLIBS_KMBOX_LIBS} KF5::AkonadiCore KF5::AkonadiKMime  KF5::KIOCore KF5::KMime)

install(TARGETS akonadi_mbox_resource DESTINATION ${PLUGIN_INSTALL_DIR})
