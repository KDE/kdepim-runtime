
add_subdirectory(wizard)

########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_mbox_resource\")

set( mboxresource_SRCS
  mboxresource_debug.cpp
  compactpage.cpp
  lockmethodpage.cpp
  deleteditemsattribute.cpp
  mboxresource.cpp
)

install( FILES mboxresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )

ki18n_wrap_ui(mboxresource_SRCS
  compactpage.ui
  lockfilepage.ui
)
kconfig_add_kcfg_files(mboxresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/mboxresource.kcfg org.kde.Akonadi.Mbox.Settings)
qt5_add_dbus_adaptor(mboxresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Mbox.Settings.xml settings.h Settings
)

add_executable(akonadi_mbox_resource ${mboxresource_SRCS})

if( APPLE )
  set_target_properties(akonadi_mbox_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_mbox_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.Mbox")
  set_target_properties(akonadi_mbox_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Mbox Resource")
endif ()

target_link_libraries(akonadi_mbox_resource KF5::Mbox KF5::AkonadiCore KF5::AkonadiMime  KF5::KIOCore KF5::Mime KF5::AkonadiAgentBase akonadi-singlefileresource)

install(TARGETS akonadi_mbox_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
if (BUILD_TESTING)
   add_subdirectory(autotests)
endif()
