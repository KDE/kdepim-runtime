set( openxchangeresource_SRCS
  oxa/connectiontestjob.cpp
  oxa/contactutils.cpp
  oxa/davmanager.cpp
  oxa/davutils.cpp
  oxa/folder.cpp
  oxa/foldercreatejob.cpp
  oxa/folderdeletejob.cpp
  oxa/foldermodifyjob.cpp
  oxa/foldermovejob.cpp
  oxa/folderrequestjob.cpp
  oxa/foldersrequestdeltajob.cpp
  oxa/foldersrequestjob.cpp
  oxa/folderutils.cpp
  oxa/incidenceutils.cpp
  oxa/object.cpp
  oxa/objectcreatejob.cpp
  oxa/objectdeletejob.cpp
  oxa/objectmodifyjob.cpp
  oxa/objectmovejob.cpp
  oxa/objectrequestjob.cpp
  oxa/objectsrequestdeltajob.cpp
  oxa/objectsrequestjob.cpp
  oxa/objectutils.cpp
  oxa/oxutils.cpp
  oxa/updateusersjob.cpp
  oxa/user.cpp
  oxa/users.cpp
  oxa/useridrequestjob.cpp
  oxa/usersrequestjob.cpp

  configdialog.cpp
  openxchangeresource.cpp
)

kde4_add_ui_files( openxchangeresource_SRCS configdialog.ui )

install( FILES openxchangeresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_kcfg_files(openxchangeresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/openxchangeresource.kcfg org.kde.Akonadi.OpenXchange.Settings)
qt4_add_dbus_adaptor(openxchangeresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.OpenXchange.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_openxchange_resource RUN_UNINSTALLED ${openxchangeresource_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_openxchange_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_openxchange_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.OpenXchange")
  set_target_properties(akonadi_openxchange_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi OpenXchange Resource")
endif (Q_WS_MAC)


target_link_libraries(akonadi_openxchange_resource ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_KABC_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KCALCORE_LIBS}  ${KDE4_AKONADI_CONTACT_LIBS} ${KDE4_AKONADI_KCAL_LIBS} )

install(TARGETS akonadi_openxchange_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(icons)
