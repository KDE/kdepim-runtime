#
#  SPDX-FileCopyrightText: 2017 Krzysztof Nowicki <krissn@op.pl>
#
#  SPDX-License-Identifier: LGPL-2.0-or-later

include_directories(../)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/fakeserver)

add_subdirectory(fakeserver)
add_subdirectory(unittests)

# The isolated tests that drive the fake server through isolatedtestcommon and the DBus
# interfaces generated for it are not built yet: their dialog entries still have to be
# ported to the fake server's new request matchers.
