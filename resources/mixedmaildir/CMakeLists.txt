project( mixedmaildirresource )

include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${kdepim-runtime_SOURCE_DIR}/resources/shared
    ${kdepim-runtime_SOURCE_DIR}/resources/shared/filestore
    ${kdepim-runtime_SOURCE_DIR}/resources/maildir
    ${kdepim-runtime_SOURCE_DIR}/resources/mbox
    ${CMAKE_CURRENT_SOURCE_DIR}/kmindexreader
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

kde_enable_exceptions()

add_subdirectory(kmindexreader)
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=5254) # TODO get our own debug area

########### next target ###############

set( mixedmaildirresource_SRCS
  compactchangehelper.cpp
  configdialog.cpp
  mixedmaildirresource.cpp
  mixedmaildirstore.cpp
  retrieveitemsjob.cpp
  ../shared/createandsettagsjob.cpp
)

install( FILES mixedmaildirresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

qt5_wrap_ui(mixedmaildirresource_SRCS settings.ui)

kconfig_add_kcfg_files(mixedmaildirresource_SRCS settings.kcfgc)

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/mixedmaildirresource.kcfg org.kde.Akonadi.MixedMaildir.Settings)

qt5_add_dbus_adaptor(mixedmaildirresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.MixedMaildir.Settings.xml settings.h Settings
)

add_executable(akonadi_mixedmaildir_resource ${mixedmaildirresource_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_mixedmaildir_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_mixedmaildir_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.MixedMaildir")
  set_target_properties(akonadi_mixedmaildir_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi MixedMaildir Resource")
endif ()

target_link_libraries(akonadi_mixedmaildir_resource
  kmindexreader
  maildir
  akonadi-filestore
  KF5::AkonadiCore
  KF5::AkonadiKMime
  
  KF5::KIOCore
  ${KDEPIMLIBS_KMBOX_LIBS}
  KF5::KMime
  KF5::PimUtils
)

install(TARGETS akonadi_mixedmaildir_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.MixedMaildir.Settings.xml
        DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})

# unit tests
if (KDE4_BUILD_TESTS)
  add_subdirectory( tests )
endif ()
