#
#  Copyright (C) 2017 Krzysztof Nowicki <krissn@op.pl>
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Library General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Library General Public License for more details.
#
#  You should have received a copy of the GNU Library General Public License
#  along with this library; see the file COPYING.LIB.  If not, write to
#  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
#  Boston, MA 02110-1301, USA.

include_directories(../)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/fakeserver)

set(isolatestestcommon_SRCS isolatedtestbase.cpp statemonitor.cpp)

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/../ewsresource.kcfg org.kde.Akonadi.Ews.Settings)
set(ewssettingsinterface_xml ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Ews.Settings.xml)
qt5_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/../ewssettings.h org.kde.Akonadi.Ews.Wallet.xml OPTIONS -a )
set(ewswalletinterface_xml ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Ews.Wallet.xml)

qt5_add_dbus_interface(isolatestestcommon_SRCS ${ewssettingsinterface_xml} ewssettings)
qt5_add_dbus_interface(isolatestestcommon_SRCS ${ewswalletinterface_xml} ewswallet)

qt5_add_resources(isolatestestcommon_RSRCS isolatedtestcommon.qrc)

add_library(isolatedtestcommon STATIC ${isolatestestcommon_SRCS})
target_link_libraries(isolatedtestcommon
  KF5::AkonadiCore
  Qt5::Core
  Qt5::Network
  Qt5::Test
  fakeewsserver
)

add_subdirectory(unittests)
add_subdirectory(fakeserver)
if (KDEPIM_RUN_AKONADI_TEST)
	add_akonadi_isolated_test_advanced(ewstest.cpp "${isolatestestcommon_RSRCS}" "isolatedtestcommon")
endif()
