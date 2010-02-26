cmake_minimum_required(VERSION 2.6)

project(qml_akonadi)

find_package(Qt4 REQUIRED)
find_package(Automoc4 REQUIRED)
# find_package(KDE4 REQUIRED)
# find_package(Akonadi REQUIRED)
# find_package(KdepimLibs REQUIRED)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

include_directories(
#     ${AKONADI_INCLUDE_DIR}
#     ${KDE4_INCLUDES}
#     ${KDEPIMLIBS_INCLUDE_DIR}
    ${QT_INCLUDES}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# include(KDE4Defaults)
# include(MacroLibrary)
# include(MacroOptionalAddSubdirectory)
# include(CheckIncludeFiles)
# include(FindKdepimLibs)

set(qml_akonadi_srcs
  main.cpp
  contactswidget.cpp
  calendarwidget.cpp
#   declarativecontactmodel.cpp
#   contactwrapper.cpp
  fakeentitytreemodel.cpp
  fakegrouplistmodel.cpp
#   eventgrouplistmodel.cpp
  eventgroupmodel.cpp
#   eventsortproxymodel.cpp
  qtselectionproxymodel.cpp
#   eventwrapper.cpp
)

automoc4_add_executable(qml_akonadi ${qml_akonadi_srcs} )

target_link_libraries(qml_akonadi
    /home/kde-devel/kde/lib/libQtDeclarative.so
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
#     ${KDEPIMLIBS_AKONADI_LIBS}
#     ${KDE4_KIO_LIBS}
#     ${KDEPIMLIBS_KABC_LIBS}
#     ${KDEPIMLIBS_AKONADI_KCAL_LIBS}
#     ${KDEPIMLIBS_KMIME_LIBS}
)

# install(TARGETS qml_akonadi ${INSTALL_TARGETS_DEFAULT_ARGS} )

# install(FILES
#   contacts.qml
#   calendar.qml
#   DESTINATION ${DATA_INSTALL_DIR}/akonadi_qml
# )
#
# install(FILES
#   content/CalendarView.qml
#   content/CloseButton.qml
#   content/DetailView.qml
#   content/PortholeView.qml
#   DESTINATION ${DATA_INSTALL_DIR}/akonadi_qml/content
# )
#
# install(FILES
#   content/pics/button.png content/pics/button-pressed.png
#   DESTINATION ${DATA_INSTALL_DIR}/akonadi_qml/content/pics
# )


## Put the data files in the build dir too, so that they can be found in QApplication::applicationDirPath()
install(FILES
  contacts.qml
  calendar.qml
  DESTINATION ${CMAKE_BINARY_DIR}
)

install(FILES
  content/CalendarView.qml
  content/CloseButton.qml
  content/DetailView.qml
  content/PortholeView.qml
  DESTINATION ${CMAKE_BINARY_DIR}/content
)

install(FILES
  content/pics/button.png content/pics/button-pressed.png
  DESTINATION ${CMAKE_BINARY_DIR}/content/pics
)
