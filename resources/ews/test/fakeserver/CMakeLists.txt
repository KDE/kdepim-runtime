#
#  SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>
#
#  SPDX-License-Identifier: LGPL-2.0-or-later


set(fakeewsserver_SRCS
    fakeewsconnection.cpp
    fakeewsserver_debug.cpp
    fakeewsserver.cpp
    fakeewsserverthread.cpp)

add_library(fakeewsserver ${fakeewsserver_SRCS})

generate_export_header(fakeewsserver BASE_NAME fakeewsserver)
target_link_libraries(fakeewsserver Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Network Qt${QT_MAJOR_VERSION}::XmlPatterns)

add_subdirectory(test)
