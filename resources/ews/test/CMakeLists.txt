#
#  SPDX-FileCopyrightText: 2017 Krzysztof Nowicki <krissn@op.pl>
#
#  SPDX-License-Identifier: LGPL-2.0-or-later

include_directories(../)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/fakeserver)

set(isolatestestcommon_SRCS
    isolatedtestbase.cpp
    isolatedtestbase.h
    statemonitor.cpp
    statemonitor.h
)

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/../ewsresource.kcfg org.kde.Akonadi.Ews.Settings)
set(ewssettingsinterface_xml ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Ews.Settings.xml)
qt_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/../ewssettings.h org.kde.Akonadi.Ews.Wallet.xml OPTIONS -a)
set(ewswalletinterface_xml ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Ews.Wallet.xml)
qt_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/../ewsresource.h org.kde.Akonadi.Ews.Resource.xml OPTIONS -a)
set(ewsresourceinterface_xml ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Ews.Resource.xml)

qt_add_dbus_interface(isolatestestcommon_SRCS ${ewssettingsinterface_xml} ewssettingsinterface)
qt_add_dbus_interface(isolatestestcommon_SRCS ${ewswalletinterface_xml} ewswalletinterface)
qt_add_dbus_interface(isolatestestcommon_SRCS ${ewsresourceinterface_xml} ewsresourceinterface)

add_library(isolatedtestcommon STATIC ${isolatestestcommon_SRCS})
target_link_libraries(
    isolatedtestcommon
    KPim6::AkonadiCore
    KPim6::AkonadiMime
    Qt::Core
    Qt::Network
    Qt::Test
    fakeewsserver
)

add_subdirectory(fakeserver)
add_subdirectory(unittests)

if(KDEPIM_RUN_AKONADI_TEST)
    add_akonadi_isolated_test(SOURCE ewstest.cpp ADDITIONAL_SOURCES isolatedtestcommon.qrc LINK_LIBRARIES isolatedtestcommon Qt::Widgets)
    add_akonadi_isolated_test(SOURCE serverbusytest.cpp ADDITIONAL_SOURCES isolatedtestcommon.qrc LINK_LIBRARIES isolatedtestcommon Qt::Widgets)
endif()
