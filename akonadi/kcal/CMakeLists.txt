include_directories(
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( kcalakonadi_LIB_SRC
     collectionselection.cpp
     calendar.cpp
     calendaradaptor.cpp
     calendarmodel.cpp
     calendarsearch.cpp
     calfilterproxymodel.cpp
     collectionselectionproxymodel.cpp
     daterangefilterproxymodel.cpp
     entitymodelstatesaver.cpp
     incidencefilterproxymodel.cpp
     kcalmodel.cpp
     kcalitembrowser.cpp
     modelstatesaver.cpp
     itembrowser.cpp
     utils.cpp
     incidenceattribute.cpp
     groupware.cpp
     identitymanager.cpp
     mailclient.cpp
     koprefs.cpp
     mailscheduler.cpp
     freebusymanager.cpp
)

kde4_add_kcfg_files(kcalakonadi_LIB_SRC koprefs_base.kcfgc )

# FIXME: cannot depend on kdepim/rutime here!
qt4_add_dbus_interface(kcalakonadi_LIB_SRC ${CMAKE_SOURCE_DIR}/runtime/agents/calendarsearch/org.freedesktop.Akonadi.CalendarSearchAgent.xml calendarsearchinterface)

kde4_add_library( akonadi-kcal_next SHARED ${kcalakonadi_LIB_SRC} )

target_link_libraries( akonadi-kcal_next  ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_AKONADI_KCAL_LIBS} ${KDEPIMLIBS_AKONADI_CONTACT_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} ${KDEPIMLIBS_KMIME_LIBS} ${KDEPIMLIBS_KPIMIDENTITIES_LIBS} ${KDEPIMLIBS_KPIMUTILS_LIBS} ${KDEPIMLIBS_MAILTRANSPORT_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS})
set_target_properties( akonadi-kcal_next PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS akonadi-kcal_next ${INSTALL_TARGETS_DEFAULT_ARGS})
