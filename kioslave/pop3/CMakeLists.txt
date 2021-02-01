########### next target ###############

set(kio_pop3_PART_SRCS pop3.cpp pop3_debug.cpp )

ecm_qt_declare_logging_category(kio_pop3_PART_SRCS HEADER pop3_debug.h IDENTIFIER POP3_LOG CATEGORY_NAME org.kde.pim.pop3
    DESCRIPTION "kioslave (kdepim-runtime)"
    OLD_CATEGORY_NAMES log_pop3resource
    EXPORT KDEPIMRUNTIME
    )



add_library(kio_pop3 MODULE ${kio_pop3_PART_SRCS})


target_link_libraries(kio_pop3 KF5::KIOCore KF5::I18n Qt5::Network Sasl2::Sasl2)
set_target_properties(kio_pop3 PROPERTIES OUTPUT_NAME "pop3")

install(TARGETS kio_pop3  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio/ )


########### install files ###############

install( FILES pop3.protocol pop3s.protocol  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )



