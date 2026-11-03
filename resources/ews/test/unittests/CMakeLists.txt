#
#  SPDX-FileCopyrightText: 2015-2018 Krzysztof Nowicki <krissn@op.pl>
#
#  SPDX-License-Identifier: LGPL-2.0-or-later

kde_enable_exceptions()

add_library(
    uttesthelpers
    STATIC
    faketransferjob.cpp
    faketransferjob.h
)
target_link_libraries(
    uttesthelpers
    Qt::Core
    KF6::KIOCore
)

macro(akonadi_ews_add_ut_advanced utname extra_SRCS)
    add_executable(
        ${utname}
        ${utname}.cpp
        ${extra_SRCS}
    )
    target_link_libraries(
        ${utname}
        Qt::Test
        uttesthelpers
    )
    add_test(NAME ${utname} COMMAND ${utname})
endmacro(
    akonadi_ews_add_ut_advanced
    utname
)

macro(akonadi_ews_add_ut utname)
    akonadi_ews_add_ut_advanced(${utname} "")
    target_link_libraries(${utname} ewsclient)
endmacro(
    akonadi_ews_add_ut
    utname
)

# These stub the HTTP transfer through a KIO::http_post override (fakehttppost.h),
# but EwsRequest has sent its requests through a TransferJob of its own since 2024,
# so the stub no longer intercepts them and every one of them fails with an unknown
# protocol. Not built until the port in draft MR !248 lands:
# ewsmoveitemrequest_ut, ewsdeleteitemrequest_ut, ewsgetitemrequest_ut and
# ewsunsubscriberequest_ut.

akonadi_ews_add_ut(ewsattachment_ut)

# The OAuth mock reimplements QAbstractOAuth and still uses Q_ENUMS, which Qt6
# removed, so it does not compile. Not built until the port lands in the same draft
# MR !248 as the request tests above.
