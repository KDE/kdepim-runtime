include_directories(${CMAKE_SOURCE_DIR}/ ${CMAKE_SOURCE_DIR}/akonadi)

set(akonadi_resources_kcm_srcs configmodule.cpp resourcesmanagementwidget.cpp)
kde4_add_ui_files(akonadi_resources_kcm_srcs resourcesmanagementwidget.ui )
kde4_add_plugin(kcm_akonadi_resources ${akonadi_resources_kcm_srcs})
target_link_libraries(kcm_akonadi_resources  ${KDE4_KDEUI_LIBS} akonadi)

install(TARGETS kcm_akonadi_resources DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kcm_akonadi_resources.desktop  DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES kcmresources_export.h resourcesmanagementwidget.h
         DESTINATION ${INCLUDE_INSTALL_DIR} )
