set(Libkolab_MODULE_DIR ${Libkolab_SOURCE_DIR}/cmake/modules)
set(CMAKE_MODULE_PATH ${Libkolab_MODULE_DIR})

set(Libkolab_VERSION_MAJOR 1)
set(Libkolab_VERSION_MINOR 0)
# Enable the full x.y.z version only for release versions
set(Libkolab_VERSION_PATCH 2)
set(Libkolab_VERSION ${Libkolab_VERSION_MAJOR}.${Libkolab_VERSION_MINOR})
set(Libkolab_VERSION_STRING ${CMAKE_PROJECT_NAME}-${Libkolab_VERSION})

if (Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

include_directories(
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

ecm_qt_declare_logging_category(KOLAB_SRCS HEADER pimkolab_debug.h IDENTIFIER PIMKOLAB_LOG CATEGORY_NAME org.kde.pim.pimkolab
    DESCRIPTION "pimkolab lib (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME
    )


set(KOLAB_LINK_LIBRARIES
    KF5::CalendarCore
    KF5::CalendarUtils
    KF5::Contacts
    KF5::Mime
    KF5::AkonadiCore
    KF5::AkonadiNotes
    KF5::Codecs
    KF5::I18n
    ${Libkolabxml_LIBRARIES}
    Qt::Core
    Qt::Xml
    Qt::Gui
    Qt::Widgets
    )

if(BUILD_TESTING)
    #for tests only
    enable_testing()
    add_library(kolab_static STATIC ${KOLAB_SRCS})
    target_link_libraries(kolab_static ${KOLAB_LINK_LIBRARIES} Qt::Test)
    add_subdirectory(autotests)
endif()

add_library(pimkolab STATIC ${KOLAB_SRCS})
target_link_libraries(pimkolab ${KOLAB_LINK_LIBRARIES})

