project(grantlee_akonadi_resource)

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

set( template_resource_SRCS
  akonaditemplateresource.cpp
)

install(FILES akonaditemplateresource.desktop
        DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_executable(akonadi_template_resource RUN_UNINSTALLED ${template_resource_SRCS})

target_link_libraries(akonadi_template_resource
                      ${KDE4_AKONADI_LIBS}
                      ${KDE4_KIO_LIBS}
                      ${AKONADI_COMMON_LIBRARIES}
                      ${QT_QTCORE_LIBRARY}
                      ${QT_QTGUI_LIBRARY}
                      ${KDEPIMLIBS_AKONADI_LIBS}
                      )

install(TARGETS akonadi_template_resource
        ${INSTALL_TARGETS_DEFAULT_ARGS})
