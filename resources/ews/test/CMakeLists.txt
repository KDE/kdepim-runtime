#
#  SPDX-FileCopyrightText: 2017 Krzysztof Nowicki <krissn@op.pl>
#
#  SPDX-License-Identifier: LGPL-2.0-or-later

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
