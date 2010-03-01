project(qml_akonadi)

find_package(KDE4 REQUIRED)
find_package(Akonadi REQUIRED)
find_package(KdepimLibs REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

include_directories(
    ${AKONADI_INCLUDE_DIR}
    ${KDE4_INCLUDES}
    ${KDEPIMLIBS_INCLUDE_DIR}
    ${QT_INCLUDES}
)

include(KDE4Defaults)
include(MacroLibrary)
include(MacroOptionalAddSubdirectory)
include(CheckIncludeFiles)
include(FindKdepimLibs)

set(qml_akonadi_contacts_srcs
  main_contacts.cpp
  contactswidget.cpp
  declarativecontactmodel.cpp
  contactwrapper.cpp
)

kde4_add_executable(qml_akonadi_contacts ${qml_akonadi_contacts_srcs} )

target_link_libraries(qml_akonadi_contacts
    QtDeclarative
    ${KDEPIMLIBS_AKONADI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDEPIMLIBS_KABC_LIBS}
    ${KDEPIMLIBS_AKONADI_KCAL_LIBS}
    ${KDEPIMLIBS_KMIME_LIBS}
)

install(TARGETS qml_akonadi_contacts ${INSTALL_TARGETS_DEFAULT_ARGS} )

set(qml_akonadi_calendar_srcs
  main_calendar.cpp
  calendarwidget.cpp
  contactwrapper.cpp
  declarativecontactmodel.cpp
  eventgrouplistmodel.cpp
  eventgroupmodel.cpp
  eventsortproxymodel.cpp
  eventwrapper.cpp
)

kde4_add_executable(qml_akonadi_calendar ${qml_akonadi_calendar_srcs} )

target_link_libraries(qml_akonadi_calendar
    QtDeclarative
    ${KDEPIMLIBS_AKONADI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDEPIMLIBS_KABC_LIBS}
    ${KDEPIMLIBS_AKONADI_KCAL_LIBS}
    ${KDEPIMLIBS_KMIME_LIBS}
)

install(TARGETS qml_akonadi_calendar ${INSTALL_TARGETS_DEFAULT_ARGS} )

if(QTVERSION VERSION_GREATER 4.6.3)
  set(contentY "contentY")
  set(properties "properties")
  set(contentHeight "contentHeight")
else()
  set(contentY "viewportY")
  set(properties "matchProperties")
  set(contentHeight "viewportHeight")
endif()

configure_file(
  ${CMAKE_SOURCE_DIR}/contacts.qml.in
  ${CMAKE_BINARY_DIR}/contacts.qml
)

configure_file(
  ${CMAKE_SOURCE_DIR}/content/DetailView.qml.in
  ${CMAKE_BINARY_DIR}/content/DetailView.qml
)

install(FILES
  ${CMAKE_BINARY_DIR}/contacts.qml
  calendar.qml
  DESTINATION ${DATA_INSTALL_DIR}/akonadi_qml
)

install(FILES
  ${CMAKE_BINARY_DIR}/content/DetailView.qml
  DESTINATION ${DATA_INSTALL_DIR}/akonadi_qml/content
)

install(FILES
  content/CalendarView.qml
  content/CloseButton.qml
  DESTINATION ${DATA_INSTALL_DIR}/akonadi_qml/content
)

install(FILES
  content/pics/button.png content/pics/button-pressed.png
  DESTINATION ${DATA_INSTALL_DIR}/akonadi_qml/content/pics
)
