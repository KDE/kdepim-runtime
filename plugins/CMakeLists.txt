project(plugins)
add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )


include_directories(
        ${Boost_INCLUDE_DIR}
)




if ("${CMAKE_SOURCE_DIR}" STREQUAL "${plugins_SOURCE_DIR}")

 set(KDEPIMLIBS_LIB_VERSION "4.70.0")

 find_package(Qt5DBus ${KF5_VERSION} CONFIG REQUIRED)

 find_package(KF5KDELibs4Support ${KF5_VERSION} CONFIG REQUIRED)


 find_package(KF5Akonadi ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
 find_package(KF5Mime ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
 find_package(KF5Abc ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
 find_package(KF5AlarmCalendar ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
 find_package(KF5CalendarCore ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
 find_package(KF5CalendarUtils ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
 find_package(KF5AkonadiContact ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
 find_package(KF5MicroBlog ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
 find_package(KF5AkonadiMime ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
endif()

find_package(KF5AkonadiAbc ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_serializer_plugins\")


set(akonadi_serializer_addressee_PART_SRCS akonadi_serializer_addressee.cpp )
add_library(akonadi_serializer_addressee MODULE ${SERIALIZER_TYPE} ${akonadi_serializer_addressee_PART_SRCS})
target_link_libraries(akonadi_serializer_addressee  KF5::Abc KF5::AkonadiCore KF5::AkonadiAbc)
install(TARGETS akonadi_serializer_addressee DESTINATION ${PLUGIN_INSTALL_DIR})

set(akonadi_serializer_mail_PART_SRCS akonadi_serializer_mail.cpp akonadi_serializer_mail_debug.cpp)
add_library(akonadi_serializer_mail MODULE ${SERIALIZER_TYPE} ${akonadi_serializer_mail_PART_SRCS})
target_link_libraries(akonadi_serializer_mail  KF5::Mime KF5::AkonadiCore KF5::AkonadiMime KF5::AkonadiPrivate Qt5::DBus )
install(TARGETS akonadi_serializer_mail DESTINATION ${PLUGIN_INSTALL_DIR})

set(akonadi_serializer_kcalcore_SRCS akonadi_serializer_kcalcore.cpp)
add_library(akonadi_serializer_kcalcore MODULE ${SERIALIZER_TYPE} ${akonadi_serializer_kcalcore_SRCS})
target_link_libraries(akonadi_serializer_kcalcore  KF5::CalendarUtils KF5::CalendarCore KF5::AkonadiCore)
install(TARGETS akonadi_serializer_kcalcore DESTINATION ${PLUGIN_INSTALL_DIR})

set(akonadi_serializer_bookmark_SRCS akonadi_serializer_bookmark.cpp)
add_library(akonadi_serializer_bookmark MODULE ${SERIALIZER_TYPE} ${akonadi_serializer_bookmark_SRCS})
target_link_libraries(akonadi_serializer_bookmark  KF5::KIOCore KF5::AkonadiCore)
install(TARGETS akonadi_serializer_bookmark DESTINATION ${PLUGIN_INSTALL_DIR})

#REACTIVATE
set(akonadi_serializer_contactgroup_PART_SRCS akonadi_serializer_contactgroup.cpp )
add_library(akonadi_serializer_contactgroup MODULE ${SERIALIZER_TYPE} ${akonadi_serializer_contactgroup_PART_SRCS})
target_link_libraries(akonadi_serializer_contactgroup  KF5::Abc KF5::AkonadiCore KF5::AkonadiContact KF5::AkonadiAbc)
install(TARGETS akonadi_serializer_contactgroup DESTINATION ${PLUGIN_INSTALL_DIR})

set( akonadi_serializer_microblog_SRCS akonadi_serializer_microblog.cpp )
add_library(akonadi_serializer_microblog MODULE ${SERIALIZER_TYPE} ${akonadi_serializer_microblog_SRCS})
target_link_libraries(akonadi_serializer_microblog KF5::AkonadiCore   ${QT_QTXML_LIBRARY} KF5::PimUtils KF5::MicroBlog)
install(TARGETS akonadi_serializer_microblog DESTINATION ${PLUGIN_INSTALL_DIR})

set(akonadi_serializer_kalarm_SRCS akonadi_serializer_kalarm.cpp kaeventformatter.cpp akonadi_serializer_kalarm_debug.cpp)
add_library(akonadi_serializer_kalarm MODULE ${SERIALIZER_TYPE} ${akonadi_serializer_kalarm_SRCS})
target_link_libraries(akonadi_serializer_kalarm
                      KF5::AlarmCalendar
                      KF5::CalendarCore
                      KF5::CalendarUtils
                      KF5::AkonadiCore
                      
                      
                     )
install(TARGETS akonadi_serializer_kalarm DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install( FILES
  akonadi_serializer_addressee.desktop
  akonadi_serializer_mail.desktop
  akonadi_serializer_kcalcore.desktop
  akonadi_serializer_bookmark.desktop
  akonadi_serializer_contactgroup.desktop
  akonadi_serializer_microblog.desktop
  akonadi_serializer_kalarm.desktop
DESTINATION ${DATA_INSTALL_DIR}/akonadi/plugins/serializer)

add_subdirectory( autotests ) 

if ("${CMAKE_BINARY_DIR}" STREQUAL "${plugins_BINARY_DIR}")
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()

