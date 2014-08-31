project(davgroupware)

if (QT_QTXMLPATTERNS_LIBRARY)
  if(WIN32)
      set(LIB_INSTALL_DIR ${LIB_INSTALL_DIR}
                          RUNTIME DESTINATION ${BIN_INSTALL_DIR}
                          LIBRARY DESTINATION ${LIB_INSTALL_DIR}
                          ARCHIVE DESTINATION ${LIB_INSTALL_DIR} )
  endif()

  include_directories(
      ${KDE4_INCLUDES}
      ${KDEPIMLIBS_INCLUDE_DIRS}
      ../common/
      ../protocols/
  )

  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


  ########### next target ###############

  set( davgroupwareresource_SRCS
    ../common/davjobbase.cpp
    ../common/davcollection.cpp
    ../common/davcollectiondeletejob.cpp
    ../common/davcollectionsfetchjob.cpp
    ../common/davcollectionmodifyjob.cpp
    ../common/davcollectionsmultifetchjob.cpp
    ../common/davprotocolbase.cpp
    ../common/davitem.cpp
    ../common/davitemcreatejob.cpp
    ../common/davitemdeletejob.cpp
    ../common/davitemfetchjob.cpp
    ../common/davitemmodifyjob.cpp
    ../common/davitemsfetchjob.cpp
    ../common/davitemslistjob.cpp
    ../common/davmanager.cpp
    ../common/davmultigetprotocol.cpp
    ../common/davprincipalhomesetsfetchjob.cpp
    ../common/davprincipalsearchjob.cpp
    ../common/davutils.cpp
    ../common/etagcache.cpp

    ../protocols/caldavprotocol.cpp
    ../protocols/carddavprotocol.cpp
    ../protocols/groupdavprotocol.cpp

    configdialog.cpp
    davfreebusyhandler.cpp
    davgroupwareresource.cpp
    davprotocolattribute.cpp
    searchdialog.cpp
    setupwizard.cpp
    settings.cpp
    urlconfigurationdialog.cpp
  )

  if(${AccountsQt_FOUND} AND ${SignOnQt_FOUND})
    include_directories(${ACCOUNTSQT_INCLUDE_DIRS} ${SIGNONQT_INCLUDE_DIRS})
    add_definitions(-DHAVE_ACCOUNTS)
    set(davgroupwareresource_SRCS ../../shared/getcredentialsjob.cpp ${davgroupwareresource_SRCS})
  endif()

  install( FILES davgroupwareresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )
  install( FILES davgroupwareprovider.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR} )

  file( GLOB providersFiles "../services/*.desktop" )
  install( FILES ${providersFiles} DESTINATION "${SERVICES_INSTALL_DIR}/akonadi/davgroupware-providers" )

  kde4_add_kcfg_files(davgroupwareresource_SRCS settingsbase.kcfgc)
  kde4_add_ui_files(davgroupwareresource_SRCS configdialog.ui urlconfigurationdialog.ui searchdialog.ui)
  kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/davgroupwareresource.kcfg org.kde.Akonadi.davGroupware.Settings)
  qt4_add_dbus_adaptor(davgroupwareresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.davGroupware.Settings.xml settings.h Settings
  )

  kde4_add_executable(akonadi_davgroupware_resource RUN_UNINSTALLED ${davgroupwareresource_SRCS})

  if (Q_WS_MAC)
    set_target_properties(akonadi_davgroupware_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../../Info.plist.template)
    set_target_properties(akonadi_davgroupware_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.davGroupware")
    set_target_properties(akonadi_davgroupware_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi davGroupware Resource")
  endif ()

  target_link_libraries(akonadi_davgroupware_resource
                        ${QT_QTCORE_LIBRARY}
                        ${QT_QTDBUS_LIBRARY}
                        ${QT_QTXML_LIBRARY}
                        ${QT_QTXMLPATTERNS_LIBRARY}
                        ${KDE4_KDECORE_LIBS}
                        ${KDE4_KIO_LIBS}
                        ${KDEPIMLIBS_AKONADI_LIBS}
                        ${KDEPIMLIBS_AKONADI_CALENDAR_LIBS}
                        ${KDEPIMLIBS_KABC_LIBS}
                        ${KDEPIMLIBS_KCALCORE_LIBS})

  if(${AccountsQt_FOUND} AND ${SignOnQt_FOUND})
    target_link_libraries(akonadi_davgroupware_resource
                        ${ACCOUNTSQT_LIBRARIES}
                        ${SIGNONQT_LIBRARIES})
  endif()
  install(TARGETS akonadi_davgroupware_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
else()
  add_feature_info("Davgroupware resource" QT_QTXMLPATTERNS_LIBRARY "The QtXmlPatterns library was not found. It is needed for building the davgroupware resource.")
endif()

