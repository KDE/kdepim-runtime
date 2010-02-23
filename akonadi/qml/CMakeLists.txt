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

set(qml_akonadi_srcs
  main.cpp
  mainwidget.cpp
  declarativecontactmodel.cpp
  contactwrapper.cpp
)

kde4_add_executable(qml_akonadi ${qml_akonadi_srcs} )

target_link_libraries(qml_akonadi
    QtDeclarative
    ${KDEPIMLIBS_AKONADI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDEPIMLIBS_KABC_LIBS}
    ${KDEPIMLIBS_KMIME_LIBS}
)

install(TARGETS qml_akonadi ${INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES
  contacts.qml
  DESTINATION ${DATA_INSTALL_DIR}/akonadi_qml
)

install(FILES
  content/CloseButton.qml
  DESTINATION ${DATA_INSTALL_DIR}/akonadi_qml/content
)

install(FILES
  content/pics/button.png content/pics/button-pressed.png
  DESTINATION ${DATA_INSTALL_DIR}/akonadi_qml/content/pics
)
