project(kioslave)
cmake_minimum_required(VERSION 2.8.12)

set(kio_akonadi_SRCS akonadislave.cpp akonadislave_debug.cpp)


add_library(kio_akonadi MODULE ${kio_akonadi_SRCS})



target_link_libraries(kio_akonadi  
                      KF5::KIOCore 
                      KF5::AkonadiCore 
                      KF5::I18n
                      Qt5::DBus
                      Qt5::Widgets
                      )
set_target_properties(kio_akonadi PROPERTIES OUTPUT_NAME "akonadi")

install(TARGETS kio_akonadi DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio )


########### install files ###############

install( FILES akonadi.protocol  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )

