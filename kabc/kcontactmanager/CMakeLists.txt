include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( kcontactmanager_SRCS
  contacteditordialog.cpp
  main.cpp
  mainwidget.cpp
  mainwindow.cpp
)

kde4_add_executable(kcontactmanager RUN_UNINSTALLED ${kcontactmanager_SRCS})

target_link_libraries(kcontactmanager ${AKONADI_LIBS} akonadi-kabc ${KDE4_KDEUI_LIBRARY} )

install(TARGETS kcontactmanager DESTINATION ${BIN_INSTALL_DIR})
install( FILES kcontactmanager.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR})
install( FILES kcontactmanagerui.rc  DESTINATION ${DATA_INSTALL_DIR}/kcontactmanager)
