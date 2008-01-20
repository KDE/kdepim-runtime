
add_definitions (-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)

add_subdirectory(tests)
add_subdirectory(icons)
add_subdirectory(pics)
add_subdirectory(kpartsdesignerplugin)
add_subdirectory(about)

#kde4_add_dcop_skels(libkdepim_SRCS ${CMAKE_SOURCE_DIR}/libkdepim/networkstatus.h )

add_definitions(${QT_QTDBUS_DEFINITIONS})



########### next target ###############

set(kdepim_LIB_SRCS
   ktimeedit.cpp 
   alarmclient.cpp
   kprefsdialog.cpp 
   kpimprefs.cpp 
   categoryselectdialog.cpp 
   categoryeditdialog.cpp 
   kdateedit.cpp 
   kimportdialog.cpp 
   kvcarddrag.cpp 
   addressesdialog.cpp 
   addresseeview.cpp 
   maillistdrag.cpp 
   kconfigpropagator.cpp 
   kconfigwizard.cpp 
   pluginloaderbase.cpp 
   addresseelineedit.cpp 
   kdatepickerpopup.cpp 
   ktimezonecombobox.cpp
   ldapclient.cpp 
   overlaywidget.cpp 
   progressmanager.cpp 
   progressdialog.cpp 
   statusbarprogresswidget.cpp 
   ssllabel.cpp 
   completionordereditor.cpp 
   diffalgo.cpp 
   addresseediffalgo.cpp 
   calendardiffalgo.cpp 
   htmldiffalgodisplay.cpp 
   broadcaststatus.cpp 
   kresourceprefs.cpp 
   kxface.cpp 
   kaccount.cpp 
   kaddrbookexternal.cpp 
   kfoldertree.cpp 
   kscoring.cpp 
   kscoringeditor.cpp 
   ksubscription.cpp 
   kwidgetlister.cpp 
   recentaddresses.cpp 
   designerfields.cpp 
   kcmdesignerfields.cpp 
   embeddedurlpage.cpp 
   kincidencechooser.cpp 
   groupwarejob.cpp 
   csshelper.cpp 
   distributionlist.cpp 
   sendsmsdialog.cpp 
   categoryhierarchyreader.cpp 
   messagestatus.cpp 
   kemailquotinghighter.cpp
   kmailcompletion.cpp
   networkstatus.cpp
   servertest.cpp
   kmeditor.cpp
   kmstylelistselectaction.cpp
   autochecktreewidget.cpp
   kmutils.cpp
   colorlistbox.cpp
   dictionarycombobox.cpp
)


QT4_ADD_DBUS_INTERFACE(kdepim_LIB_SRCS ${CMAKE_SOURCE_DIR}/kaddressbook/org.kde.KAddressbook.Core.xml kaddressbookcore_interface )

QT4_ADD_DBUS_INTERFACE(kdepim_LIB_SRCS ${CMAKE_SOURCE_DIR}/korganizer/korgac/org.kde.korganizer.KOrgac.xml korganizer_korgac_interface )

kde4_add_ui_files(kdepim_LIB_SRCS categoryselectdialog_base.ui categoryeditdialog_base.ui addresspicker.ui)

#kde4_add_dcop_skels(kdepim_LIB_SRCS 
#	${CMAKE_SOURCE_DIR}/libkdepim/interfaces/MailTransportServiceIface.h 
#	${CMAKE_SOURCE_DIR}/libkdepim/interfaces/AddressBookServiceIface.h
#	addresseelineedit.h)

kde4_add_library(kdepim SHARED ${kdepim_LIB_SRCS})

target_link_libraries(kdepim  ${KDE4_KDE3SUPPORT_LIBS} ${QT_QTDESIGNER_LIBRARY} ${KDE4_KRESOURCES_LIBS} ${KDE4_KABC_LIBS} ${KDE4_KCAL_LIBS}  kimproxy ${KDE4_KPIMUTILS_LIBS} ${KDE4_KLDAP_LIBS} ${KDE4_KPIMIDENTITIES_LIBS})

set_target_properties(kdepim PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})
install(TARGETS kdepim  DESTINATION ${LIB_INSTALL_DIR})


########### next target ###############

set(kdepimwidgets_PART_SRCS) 
kde4_add_widget_files(kdepimwidgets_PART_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/kdepim.widgets)

kde4_add_plugin(kdepimwidgets ${kdepimwidgets_PART_SRCS})



target_link_libraries(kdepimwidgets  ${KDE4_KIO_LIBS} kdepim)

install(TARGETS kdepimwidgets  DESTINATION ${PLUGIN_INSTALL_DIR}/plugins/designer)


########### install files ###############

install(FILES kdepimprotocols.h  DESTINATION ${INCLUDE_INSTALL_DIR})

install(FILES interfaces/org.kde.addressbook.service.xml interfaces/org.kde.mailtransport.service.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR}) 
