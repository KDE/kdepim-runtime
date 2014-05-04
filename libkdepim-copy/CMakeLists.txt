
project(libkdepim-copy)
#add_subdirectory( tests )
add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

#add_definitions(-DKDE_DEFAULT_DEBUG_AREA=5300)

########### next target ###############

find_package(KF5KDELibs4Support ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarCore ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarUtils ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5PimUtils ${KF5_VERSION} CONFIG REQUIRED)

set(kdepim-copy_LIB_SRCS
    calendardiffalgo.cpp
    diffalgo.cpp
    htmldiffalgodisplay.cpp
    kincidencechooser.cpp
)

add_library(kdepim-copy ${LIBRARY_TYPE} ${kdepim-copy_LIB_SRCS})

target_link_libraries(kdepim-copy
  KF5::CalendarCore
  KF5::CalendarUtils
  KF5::KDELibs4Support
)

set_target_properties(kdepim-copy PROPERTIES VERSION ${KF5_VERSION} SOVERSION ${KF5_VERSION})
install(TARGETS kdepim-copy ${INSTALL_TARGETS_DEFAULT_ARGS})
