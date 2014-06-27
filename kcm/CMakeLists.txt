project(kcmakonadi)
add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )


if ("${CMAKE_SOURCE_DIR}" STREQUAL "${kcmakonadi_SOURCE_DIR}")
  set(KDEPIMLIBS_LIB_VERSION "4.70.0")
  find_package(KF5KCMUtils ${KF5_VERSION} CONFIG REQUIRED)
  find_package(KF5KDELibs4Support ${KF5_VERSION} CONFIG REQUIRED)
  find_package(KF5Akonadi ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
endif()

set(akonadi_resources_kcm_srcs configmodule.cpp resourcesmanagementwidget.cpp)
ki18n_wrap_ui(akonadi_resources_kcm_srcs resourcesmanagementwidget.ui )
add_library(kcm_akonadi_resources MODULE ${akonadi_resources_kcm_srcs})
target_link_libraries(kcm_akonadi_resources  KF5::AkonadiCore KF5::AkonadiWidgets Qt5::Widgets)


set(akonadi_server_kcm_srcs serverconfigmodule.cpp)
ki18n_wrap_ui(akonadi_server_kcm_srcs serverconfigmodule.ui
                                          servermysqlstorage.ui
                                          serverpsqlstorage.ui
                                          serverstoragedriver.ui)
add_library(kcm_akonadi_server MODULE ${akonadi_server_kcm_srcs})
target_link_libraries(kcm_akonadi_server  KF5::KIOCore KF5::AkonadiCore KF5::KCMUtils)


set(akonadi_kcm_srcs akonadiconfigmodule.cpp)
add_library(kcm_akonadi MODULE ${akonadi_kcm_srcs})
target_link_libraries(kcm_akonadi  KF5::KCMUtils KF5::AkonadiWidgets KF5::AkonadiCore)


install(TARGETS
  kcm_akonadi_resources
  kcm_akonadi_server
  kcm_akonadi
  DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES
  kcm_akonadi_resources.desktop
  kcm_akonadi_server.desktop
  kcm_akonadi.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${kcmakonadi_SOURCE_DIR}")
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()

