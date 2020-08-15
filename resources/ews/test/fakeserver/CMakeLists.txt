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

target_link_libraries(fakeewsserver Qt5::Core Qt5::Network Qt5::XmlPatterns)

add_subdirectory(test)
