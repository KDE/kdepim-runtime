########### next target ###############

add_library(kio_pop3 MODULE)
target_sources(kio_pop3 PRIVATE pop3.cpp )

ecm_qt_declare_logging_category(kio_pop3 HEADER pop3_debug.h IDENTIFIER POP3_LOG CATEGORY_NAME org.kde.pim.pop3
    DESCRIPTION "kioslave (kdepim-runtime)"
    OLD_CATEGORY_NAMES log_pop3resource
    EXPORT KDEPIMRUNTIME
    )





target_link_libraries(kio_pop3 KF5::KIOCore KF5::I18n Qt::Network Sasl2::Sasl2)
set_target_properties(kio_pop3 PROPERTIES OUTPUT_NAME "pop3")

install(TARGETS kio_pop3  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio/ )


########### install files ###############

install( FILES pop3.protocol pop3s.protocol  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )



