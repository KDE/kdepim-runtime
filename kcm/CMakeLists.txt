include_directories(${CMAKE_SOURCE_DIR}/ ${CMAKE_SOURCE_DIR}/akonadi)

set(rfc822_kcm_srcs configmodule.cpp rfc822managementwidget.cpp)
kde4_add_ui_files(rfc822_kcm_srcs rfc822managementwidget.ui )
kde4_add_plugin(kcm_rfc822 ${rfc822_kcm_srcs})
target_link_libraries(kcm_rfc822  ${KDE4_KDEUI_LIBS} akonadi)

install(TARGETS kcm_rfc822 DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kcm_rfc822.desktop  DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES rfc822_export.h rfc822managementwidget.h
         DESTINATION ${INCLUDE_INSTALL_DIR} )
