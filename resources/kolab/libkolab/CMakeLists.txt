remove_definitions(
                   -DQT_NO_CAST_FROM_ASCII
)

set(Libkolab_MODULE_DIR ${Libkolab_SOURCE_DIR}/cmake/modules)
set(CMAKE_MODULE_PATH ${Libkolab_MODULE_DIR})

# Versioning
# x.y.z scheme
# Development versions are only x.y
#
# i.e.
# 0.1 (0.1 development version towards 0.1.0)
# 0.1.0 (first release)
# 0.1.1 (patch release for 0.1.0)
# 0.2 (0.2 development version towards 0.2.0)
set(Libkolab_VERSION_MAJOR 1)
set(Libkolab_VERSION_MINOR 0)
# Enable the full x.y.z version only for release versions
set(Libkolab_VERSION_PATCH 2)
#set(Libkolab_VERSION ${Libkolab_VERSION_MAJOR}.${Libkolab_VERSION_MINOR}.${Libkolab_VERSION_PATCH})
set(Libkolab_VERSION ${Libkolab_VERSION_MAJOR}.${Libkolab_VERSION_MINOR})
set(Libkolab_VERSION_STRING ${CMAKE_PROJECT_NAME}-${Libkolab_VERSION})

# set up install directories.
set(LIB_INSTALL_DIR lib${LIB_SUFFIX} CACHE STRING "The directories where to install libraries to")
set(INCLUDE_INSTALL_DIR include CACHE STRING "The directory where to install headers to")
set(INCLUDE_INSTALL_DIR ${INCLUDE_INSTALL_DIR}/kolab)
set(CMAKECONFIG_INSTALL_DIR ${LIB_INSTALL_DIR}/cmake/Libkolab)

if (Qt5_POSITION_INDEPENDENT_CODE)
    #Requires cmake 2.8.9 (same as -fPIC on gcc)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

find_package(Boost REQUIRED)

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/kolabformatV2
    ${Libkolabxml_INCLUDES}
    ./
)

configure_file(libkolab-version.h.cmake "${CMAKE_CURRENT_BINARY_DIR}/libkolab-version.h" @ONLY)

add_subdirectory(utils)

set(KOLAB_SRCS
    kolabformat/kolabobject.cpp
    kolabformat/xmlobject.cpp
    kolabformat/formathelpers.cpp
    kolabformat/errorhandler.cpp
    kolabformat/v2helpers.cpp
    kolabformat/mimeobject.cpp
    mime/mimeutils.cpp
    utils/porting.cpp
    conversion/kcalconversion.cpp
    conversion/kabcconversion.cpp
    conversion/commonconversion.cpp
    conversion/kolabconversion.cpp
    conversion/timezoneconverter.cpp
    calendaring/calendaring.cpp
    calendaring/event.cpp
    calendaring/datetimeutils.cpp
    icalendar/icalendar.cpp
    icalendar/imip.cpp
    kolabformatV2/kolabbase.cpp
    kolabformatV2/contact.cpp
    kolabformatV2/distributionlist.cpp
    kolabformatV2/event.cpp
    kolabformatV2/task.cpp
    kolabformatV2/journal.cpp
    kolabformatV2/incidence.cpp
    kolabformatV2/note.cpp
    freebusy/freebusy.cpp
)

set(KOLAB_LINK_LIBRARIES
    KF5::CalendarCore
    KF5::CalendarUtils
    KF5::Contacts
    KF5::Mime
    KF5::AkonadiCore
    KF5::AkonadiNotes
    KF5::Codecs
    KF5::KDELibs4Support
    ${Libkolabxml_LIBRARIES}
    Qt5::Core
    Qt5::Xml
    Qt5::Gui
    Qt5::Widgets
)

if(BUILD_TESTING)
    #for tests only
    enable_testing()
    add_library(kolab_static STATIC ${KOLAB_SRCS})
    target_link_libraries(kolab_static ${KOLAB_LINK_LIBRARIES} Qt5::Test)
    add_subdirectory(autotests)
endif()

add_library(pimkolab STATIC ${KOLAB_SRCS})
target_link_libraries(pimkolab ${KOLAB_LINK_LIBRARIES})

