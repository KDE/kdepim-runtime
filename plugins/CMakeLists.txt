add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )


add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_serializer_plugins\")


set(akonadi_serializer_addressee_PART_SRCS akonadi_serializer_addressee.cpp )
add_library(akonadi_serializer_addressee MODULE ${SERIALIZER_TYPE} ${akonadi_serializer_addressee_PART_SRCS})
target_link_libraries(akonadi_serializer_addressee
    KF5::Contacts
    KF5::AkonadiCore
    KF5::AkonadiSearch
    KF5::I18n
)
install(TARGETS akonadi_serializer_addressee DESTINATION ${KDE_INSTALL_PLUGINDIR})


set(akonadi_serializer_mail_PART_SRCS akonadi_serializer_mail.cpp)
ecm_qt_declare_logging_category(akonadi_serializer_mail_PART_SRCS HEADER akonadi_serializer_mail_debug.h IDENTIFIER AKONADI_SERIALIZER_MAIL_LOG CATEGORY_NAME org.kde.pim.akonadi_serializer_mail)
add_library(akonadi_serializer_mail MODULE ${SERIALIZER_TYPE} ${akonadi_serializer_mail_PART_SRCS})
target_link_libraries(akonadi_serializer_mail
    KF5::Mime
    KF5::AkonadiCore
    KF5::AkonadiMime
    KF5::AkonadiPrivate
    KF5::AkonadiSearch
    Qt5::DBus
)
install(TARGETS akonadi_serializer_mail DESTINATION ${KDE_INSTALL_PLUGINDIR})


set(akonadi_serializer_kcalcore_SRCS akonadi_serializer_kcalcore.cpp)
add_library(akonadi_serializer_kcalcore MODULE ${SERIALIZER_TYPE} ${akonadi_serializer_kcalcore_SRCS})
target_link_libraries(akonadi_serializer_kcalcore 
    KF5::CalendarUtils
    KF5::CalendarCore
    KF5::AkonadiCore
    KF5::AkonadiSearch
    KF5::I18n
)
install(TARGETS akonadi_serializer_kcalcore DESTINATION ${KDE_INSTALL_PLUGINDIR})


set(akonadi_serializer_contactgroup_PART_SRCS akonadi_serializer_contactgroup.cpp )
add_library(akonadi_serializer_contactgroup MODULE ${SERIALIZER_TYPE} ${akonadi_serializer_contactgroup_PART_SRCS})
target_link_libraries(akonadi_serializer_contactgroup
    KF5::Contacts
    KF5::AkonadiCore
    KF5::AkonadiSearch
    KF5::I18n
    KF5::AkonadiContact
)
install(TARGETS akonadi_serializer_contactgroup DESTINATION ${KDE_INSTALL_PLUGINDIR})


set(akonadi_serializer_kalarm_SRCS akonadi_serializer_kalarm.cpp kaeventformatter.cpp)
ecm_qt_declare_logging_category(akonadi_serializer_kalarm_SRCS HEADER akonadi_serializer_kalarm_debug.h IDENTIFIER AKONADI_SERIALIZER_KALARM_LOG CATEGORY_NAME org.kde.pim.akonadi_serializer_kalarm)
add_library(akonadi_serializer_kalarm MODULE ${SERIALIZER_TYPE} ${akonadi_serializer_kalarm_SRCS})
target_link_libraries(akonadi_serializer_kalarm
    KF5::AlarmCalendar
    KF5::CalendarCore
    KF5::CalendarUtils
    KF5::AkonadiCore
    KF5::AkonadiSearch
)
install(TARGETS akonadi_serializer_kalarm DESTINATION ${KDE_INSTALL_PLUGINDIR})


set(akonadi_serializer_collection_SRCS akonadi_serializer_collection.cpp)
add_library(akonadi_serializer_collection MODULE ${SERIALIZER_TYPE} ${akonadi_serializer_collection_SRCS})
target_link_libraries(akonadi_serializer_collection
    KF5::AkonadiCore
    KF5::AkonadiSearch
)
install(TARGETS akonadi_serializer_collection DESTINATION ${KDE_INSTALL_PLUGINDIR})

########### install files ###############

install( FILES
  akonadi_serializer_addressee.desktop
  akonadi_serializer_mail.desktop
  akonadi_serializer_kcalcore.desktop
  akonadi_serializer_contactgroup.desktop
  akonadi_serializer_kalarm.desktop
  akonadi_serializer_collection.desktop
DESTINATION ${KDE_INSTALL_DATADIR}/akonadi/plugins/serializer)

if (BUILD_TESTING)
   add_subdirectory( autotests ) 
endif()

