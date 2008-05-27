include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( kcontactmanager_SRCS
  collectioncombobox.cpp
  contacteditordialog.cpp
  main.cpp
  mainwidget.cpp
  mainwindow.cpp
)


# todo: more appropriate icon?
kde4_add_app_icon(kcontactmanager_SRCS "${KDE4_ICON_DIR}/oxygen/*/actions/view-pim-contacts.png")

kde4_add_executable(kcontactmanager RUN_UNINSTALLED ${kcontactmanager_SRCS})

target_link_libraries(kcontactmanager ${KDE4_AKONADI_LIBS} akonadi-kabc ${KDE4_KDEUI_LIBRARY} )

install(TARGETS kcontactmanager ${INSTALL_TARGETS_DEFAULT_ARGS})
install( FILES kcontactmanager.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR})
install( FILES kcontactmanagerui.rc  DESTINATION ${DATA_INSTALL_DIR}/kcontactmanager)
