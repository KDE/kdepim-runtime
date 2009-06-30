project(akonadi_template_loader)

find_package(KDE4 REQUIRED)
find_package(Akonadi REQUIRED)
find_package(KdepimLibs REQUIRED)

include_directories(
  ${AKONADI_INCLUDE_DIR}
  ${KDE4_INCLUDES}
  ${KDEPIMLIBS_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${PROJECT_BINARY_DIR}
)

set( akonadi_template_loader_SRCS
  akonaditemplateloader.cpp
)

automoc4_add_library(akonadi_template_loader SHARED
  ${akonadi_template_loader_SRCS}
)

target_link_libraries(akonadi_template_loader
  akonadi_next
  ${QT_QTCORE_LIBRARY}
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDE4_KIO_LIBS}
  ${AKONADI_COMMON_LIBRARIES}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${KDEPIMLIBS_AKONADI_LIBS}
  grantlee_core_library
)

set_target_properties(akonadi_template_loader PROPERTIES
  VERSION    ${GRANTLEE_VERSION_MAJOR}.${GRANTLEE_VERSION_MINOR}.${GRANTLEE_VERSION_PATCH}
  SOVERSION  0
)

install(TARGETS akonadi_template_loader
         RUNTIME DESTINATION ${BIN_INSTALL_DIR}
         LIBRARY DESTINATION ${LIB_INSTALL_DIR}
         ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
         COMPONENT Devel
)
