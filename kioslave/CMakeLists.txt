project(kioslave)


set(KF5_VERSION "4.98.0")
set(QT_REQUIRED_VERSION "5.2.0")


find_package(KF5KIO ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Akonadi ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5ItemModels ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5KDELibs4Support ${KF5_VERSION} CONFIG REQUIRED)

set(kio_akonadi_srcs akonadislave.cpp )


add_library(kio_akonadi MODULE ${kio_akonadi_srcs})



target_link_libraries(kio_akonadi  KF5::KDELibs4Support KF5::ItemModels KF5::KIOCore KF5::AkonadiCore )

install(TARGETS kio_akonadi DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES akonadi.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )


