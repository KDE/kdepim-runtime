#
#  SPDX-FileCopyrightText: 2017 Krzysztof Nowicki <krissn@op.pl>
#
#  SPDX-License-Identifier: LGPL-2.0-or-later

include_directories(../)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/fakeserver)

add_subdirectory(unittests)

# The fake EWS server is built on QXmlQuery, which went away with XmlPatterns in
# Qt6, so it is not built, and neither are the isolated tests that drive it through
# isolatedtestcommon and the DBus interfaces generated for it. Porting the server is
# a work of its own.
