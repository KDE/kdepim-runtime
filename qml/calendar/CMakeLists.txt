project(calendar)

set(calendar_SRCS
    calendarplugin.cpp
    calendar.cpp
    calendardata.cpp
    calendardayhelper.cpp
    calendarroleproxymodel.cpp
    datetimerangefiltermodel.cpp
    daysmodel.cpp
)

INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${KDE4_INCLUDES}
)

qt4_automoc(${calendar_SRCS})

kde4_add_library(calendarplugin SHARED ${calendar_SRCS})

target_link_libraries(calendarplugin
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDECLARATIVE_LIBRARY}
    ${KDEPIMLIBS_AKONADI_LIBS}
    ${KDEPIMLIBS_KCALCORE_LIBS}
    ${KDEPIMLIBS_AKONADI_CALENDAR_LIBS}
)

install(TARGETS calendarplugin DESTINATION ${IMPORTS_INSTALL_DIR}/org/kde/pim/calendar)
install(FILES qmldir DESTINATION ${IMPORTS_INSTALL_DIR}/org/kde/pim/calendar)

