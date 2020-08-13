#
#  SPDX-FileCopyrightText: 2015-2018 Krzysztof Nowicki <krissn@op.pl>
#
#  SPDX-License-Identifier: LGPL-2.0-or-later

kde_enable_exceptions()

add_library(uttesthelpers STATIC faketransferjob.cpp)
target_link_libraries(uttesthelpers Qt5::Core KF5::KIOCore)

macro(akonadi_ews_add_ut_advanced utname extra_SRCS)
  add_executable(${utname} ${utname}.cpp ${extra_SRCS})
  target_link_libraries(${utname} Qt5::Test uttesthelpers)
  add_test(NAME ${utname} COMMAND ${utname})
endmacro(akonadi_ews_add_ut_advanced utname)

macro(akonadi_ews_add_ut utname)
  akonadi_ews_add_ut_advanced(${utname} "")
  target_link_libraries(${utname} ewsclient)
endmacro(akonadi_ews_add_ut utname)

akonadi_ews_add_ut(ewsmoveitemrequest_ut)
akonadi_ews_add_ut(ewsdeleteitemrequest_ut)
akonadi_ews_add_ut(ewsgetitemrequest_ut)
akonadi_ews_add_ut(ewsunsubscriberequest_ut)
akonadi_ews_add_ut(ewsattachment_ut)

qt5_wrap_cpp(ewssettings_ut_SRCS ewssettings_ut_mock.h)
akonadi_ews_add_ut_advanced(ewssettings_ut "${CMAKE_CURRENT_SOURCE_DIR}/../../ewssettings.cpp;${ewssettings_ut_SRCS}")
target_link_libraries(ewssettings_ut
  KF5::AkonadiCore
  KF5::WidgetsAddons
  KF5::I18n
  KF5::ConfigCore
  KF5::ConfigGui
  KF5::CoreAddons
  KF5::Wallet)
target_compile_definitions(ewssettings_ut PUBLIC -DEWSSETTINGS_UNITTEST)

if (Qt5NetworkAuth_FOUND)
  set(ewsoauth_ut_SRCS
    ewsoauth_ut_mock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ewsclient/auth/ewsabstractauth.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ewsclient/auth/ewsoauth.cpp)
  akonadi_ews_add_ut_advanced(ewsoauth_ut "${ewsoauth_ut_SRCS}")
  target_link_libraries(ewsoauth_ut
    Qt5::Widgets
    KF5::I18n
    Qt5::WebEngineWidgets
    )
  target_compile_definitions(ewsoauth_ut PUBLIC -DEWSOAUTH_UNITTEST)
endif ()
