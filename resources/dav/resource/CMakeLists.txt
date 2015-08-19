project(davgroupware)

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_davgroupware_resource\")

#if (QT_QTXMLPATTERNS_LIBRARY)
  if(WIN32)
      set(LIB_INSTALL_DIR ${KDE_INSTALL_LIBDIR}
                          RUNTIME DESTINATION ${KDE_INSTALL_BINDIR}
                          LIBRARY DESTINATION ${KDE_INSTALL_LIBDIR}
                          ARCHIVE DESTINATION ${KDE_INSTALL_LIBDIR} )
  endif()

  include_directories(
      ../common/
      ../protocols/
  )

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
    davresource_debug.cpp
    configdialog.cpp
    ctagattribute.cpp
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
    set(davgroupwareresource_SRCS ../../shared/singlefileresource/getcredentialsjob.cpp ${davgroupwareresource_SRCS})
  endif()

  install( FILES davgroupwareresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )
  install( FILES davgroupwareprovider.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR} )

  file( GLOB providersFiles "../services/*.desktop" )
  install( FILES ${providersFiles} DESTINATION "${KDE_INSTALL_KSERVICES5DIR}/akonadi/davgroupware-providers" )

  kconfig_add_kcfg_files(davgroupwareresource_SRCS settingsbase.kcfgc)
  ki18n_wrap_ui(davgroupwareresource_SRCS configdialog.ui urlconfigurationdialog.ui searchdialog.ui)
  kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/davgroupwareresource.kcfg org.kde.Akonadi.davGroupware.Settings)
  qt5_add_dbus_adaptor(davgroupwareresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.davGroupware.Settings.xml settings.h Settings
  )

  add_executable(akonadi_davgroupware_resource ${davgroupwareresource_SRCS})

  if( APPLE )
    set_target_properties(akonadi_davgroupware_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../../Info.plist.template)
    set_target_properties(akonadi_davgroupware_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.davGroupware")
    set_target_properties(akonadi_davgroupware_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi davGroupware Resource")
  endif ()

  target_link_libraries(akonadi_davgroupware_resource
          Qt5::XmlPatterns
          KF5::Contacts
	  KF5::AkonadiCalendar
          KF5::KIOCore
	  KF5::AkonadiAgentBase
          KF5::AkonadiCore
          KF5::Wallet
          KF5::CalendarCore)

  if(${AccountsQt_FOUND} AND ${SignOnQt_FOUND})
    target_link_libraries(akonadi_davgroupware_resource
                        ${ACCOUNTSQT_LIBRARIES}
                        ${SIGNONQT_LIBRARIES})
  endif()
  install(TARGETS akonadi_davgroupware_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
#else()
#  add_feature_info("Davgroupware resource" QT_QTXMLPATTERNS_LIBRARY "The QtXmlPatterns library was not found. It is needed for building the davgroupware resource.")
#endif()

