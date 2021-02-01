set(kio_akonadi_SRCS akonadislave.cpp)
ecm_qt_declare_logging_category(kio_akonadi_SRCS HEADER akonadislave_debug.h IDENTIFIER AKONADISLAVE_LOG CATEGORY_NAME org.kde.pim.akonadislave
    DESCRIPTION "kioslave (akonadi)"
    OLD_CATEGORY_NAMES log_akonadislave
    EXPORT KDEPIMRUNTIME
    )


add_library(kio_akonadi MODULE ${kio_akonadi_SRCS})



target_link_libraries(kio_akonadi  
                      KF5::KIOCore 
                      KF5::AkonadiCore 
                      KF5::I18n
                      Qt5::DBus
                      Qt5::Widgets
                      Qt5::Network
                      )
set_target_properties(kio_akonadi PROPERTIES OUTPUT_NAME "akonadi")

install(TARGETS kio_akonadi DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio )


########### install files ###############

install( FILES akonadi.protocol  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )

