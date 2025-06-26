#
#  SPDX-FileCopyrightText: 2015-2018 Krzysztof Nowicki <krissn@op.pl>
#
#  SPDX-License-Identifier: LGPL-2.0-or-later

kde_enable_exceptions()

add_library(uttesthelpers STATIC faketransferjob.cpp faketransferjob.h)
target_link_libraries(uttesthelpers Qt::Core KF6::KIOCore)

macro(akonadi_ews_add_ut_advanced utname extra_SRCS)
  add_executable(${utname} ${utname}.cpp ${extra_SRCS})
  target_link_libraries(${utname} Qt::Test uttesthelpers)
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

if(TARGET Qt::NetworkAuth)
  set(ewsoauth_ut_SRCS
    ewsoauth_ut_mock.cpp ewsoauth_ut_mock.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ewsclient/auth/ewsabstractauth.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ewsclient/auth/ewsoauth.cpp)
  akonadi_ews_add_ut_advanced(ewsoauth_ut "${ewsoauth_ut_SRCS}")
  target_link_libraries(ewsoauth_ut
    Qt::Widgets
    KF6::I18n
    Qt::WebEngineWidgets
)
  target_compile_definitions(ewsoauth_ut PUBLIC -DEWSOAUTH_UNITTEST)
endif()
