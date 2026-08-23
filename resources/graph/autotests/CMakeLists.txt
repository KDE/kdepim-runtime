# SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
# SPDX-License-Identifier: LGPL-2.0-or-later

include(ECMMarkAsTest)

macro(add_graph_test _name)
    # graph_common_SRCS carries the generated logging category (graph_debug.h/.cpp)
    # from the parent directory, which the handler sources include.
    add_executable(
        ${_name}
        ${ARGN}
        ${graph_common_SRCS}
    )
    add_test(NAME graph-${_name} COMMAND ${_name})
    ecm_mark_as_test(${_name})
    target_include_directories(
        ${_name}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/..
            ${CMAKE_CURRENT_BINARY_DIR}/..
    )
    target_link_libraries(
        ${_name}
        Qt::Test
        KPim6::AkonadiCore
        KPim6::AkonadiMime
        KF6::Mime
        KF6::CalendarCore
        KF6::Contacts
    )
endmacro()

add_graph_test(grapheventhandlertest grapheventhandlertest.cpp ../calendar/grapheventhandler.cpp)
add_graph_test(graphtodohandlertest graphtodohandlertest.cpp ../todo/graphtodohandler.cpp ../calendar/grapheventhandler.cpp)
add_graph_test(graphcontacthandlertest graphcontacthandlertest.cpp ../contact/graphcontacthandler.cpp)
add_graph_test(graphmailhandlertest graphmailhandlertest.cpp ../mail/graphmailhandler.cpp)
add_graph_test(graphsyncstateattributetest graphsyncstateattributetest.cpp ../graphsyncstateattribute.cpp)
