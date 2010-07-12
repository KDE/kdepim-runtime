include_directories(
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/akonadi
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

if (QT_QTDECLARATIVE_FOUND AND KDEPIM_MOBILE_UI)
 add_subdirectory(qml)
endif (QT_QTDECLARATIVE_FOUND AND KDEPIM_MOBILE_UI)

########### next target ###############

set( kcalakonadi_LIB_SRC
     collectionselection.cpp
     calendar.cpp
     calendaradaptor.cpp
     calendarmodel.cpp
     calendarsearch.cpp
     calfilterproxymodel.cpp
     daterangefilterproxymodel.cpp
     incidencefilterproxymodel.cpp
     kcalmodel.cpp
     incidenceviewer.cpp
     utils.cpp
     groupware.cpp
     identitymanager.cpp
     mailclient.cpp
     kcalprefs.cpp
     mailscheduler.cpp
     freebusymanager.cpp
     dndfactory.cpp
     incidencechanger.cpp
)

kde4_add_kcfg_files(kcalakonadi_LIB_SRC kcalprefs_base.kcfgc )

qt4_add_dbus_interface(kcalakonadi_LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/org.freedesktop.Akonadi.CalendarSearchAgent.xml calendarsearchinterface)

kde4_add_library( akonadi-kcal_next SHARED ${kcalakonadi_LIB_SRC} )

target_link_libraries( akonadi-kcal_next  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_AKONADI_KCAL_LIBS}
  ${KDEPIMLIBS_AKONADI_CONTACT_LIBS}
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${KDEPIMLIBS_KPIMIDENTITIES_LIBS}
  ${KDEPIMLIBS_KPIMUTILS_LIBS}
  ${KDEPIMLIBS_MAILTRANSPORT_LIBS}
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  akonadi_next kdepimdbusinterfaces
)

set_target_properties( akonadi-kcal_next PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS akonadi-kcal_next ${INSTALL_TARGETS_DEFAULT_ARGS})
