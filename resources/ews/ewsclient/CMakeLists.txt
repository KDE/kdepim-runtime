#
#  SPDX-FileCopyrightText: 2015-2018 Krzysztof Nowicki <krissn@op.pl>
#
#  SPDX-License-Identifier: LGPL-2.0-or-later

include_directories(${CMAKE_CURRENT_BINARY_DIR}/../)

set(EWSCLIENT_SRCS
    auth/ewsabstractauth.cpp
    auth/ewspasswordauth.cpp
    ewsattachment.cpp
    ewsattendee.cpp
    ewsclient.cpp
    ewscreatefolderrequest.cpp
    ewscreateitemrequest.cpp
    ewsdeletefolderrequest.cpp
    ewsdeleteitemrequest.cpp
    ewseffectiverights.cpp
    ewseventrequestbase.cpp
    ewsfindfolderrequest.cpp
    ewsfinditemrequest.cpp
    ewsfolder.cpp
    ewsfoldershape.cpp
    ewsgeteventsrequest.cpp
    ewsgetstreamingeventsrequest.cpp
    ewsgetfolderrequest.cpp
    ewsgetitemrequest.cpp
    ewsid.cpp
    ewsitem.cpp
    ewsitembase.cpp
    ewsitemshape.cpp
    ewsjob.cpp
    ewsmailbox.cpp
    ewsmovefolderrequest.cpp
    ewsmoveitemrequest.cpp
    ewsoccurrence.cpp
    ewspoxautodiscoverrequest.cpp
    ewspropertyfield.cpp
    ewsrecurrence.cpp
    ewsrequest.cpp
    ewsserverversion.cpp
    ewssubscriberequest.cpp
    ewssyncfolderhierarchyrequest.cpp
    ewssyncfolderitemsrequest.cpp
    ewstypes.cpp
    ewsunsubscriberequest.cpp
    ewsupdatefolderrequest.cpp
    ewsupdateitemrequest.cpp
    ewsxml.cpp
    ewsclient_debug.cpp
    auth/ewsabstractauth.h
    auth/ewspasswordauth.h
    ewsattachment.h
    ewsattendee.h
    ewsclient.h
    ewscreatefolderrequest.h
    ewscreateitemrequest.h
    ewsdeletefolderrequest.h
    ewsdeleteitemrequest.h
    ewseffectiverights.h
    ewseventrequestbase.h
    ewsfindfolderrequest.h
    ewsfinditemrequest.h
    ewsfolder.h
    ewsfoldershape.h
    ewsgeteventsrequest.h
    ewsgetstreamingeventsrequest.h
    ewsgetfolderrequest.h
    ewsgetitemrequest.h
    ewsid.h
    ewsitem.h
    ewsitembase.h
    ewsitemshape.h
    ewsjob.h
    ewsmailbox.h
    ewsmovefolderrequest.h
    ewsmoveitemrequest.h
    ewsoccurrence.h
    ewspoxautodiscoverrequest.h
    ewspropertyfield.h
    ewsrecurrence.h
    ewsserverversion.h
    ewssubscriberequest.h
    ewssyncfolderhierarchyrequest.h
    ewssyncfolderitemsrequest.h
    ewstypes.h
    ewsunsubscriberequest.h
    ewsupdatefolderrequest.h
    ewsupdateitemrequest.h
    ewsxml.h


    )

if (TARGET Qt${QT_MAJOR_VERSION}::NetworkAuth)
    list(APPEND EWSCLIENT_SRCS
        auth/ewsoauth.cpp
        auth/ewsoauth.h)
endif ()

if (Qca-qt5_FOUND)
    list(APPEND EWSCLIENT_SRCS
        auth/ewspkeyauthjob.cpp
        auth/ewspkeyauthjob.h)
endif ()

ecm_qt_declare_logging_category(EWSCLIENT_SRCS
    HEADER ewscli_debug.h
    IDENTIFIER EWSCLI_LOG
    CATEGORY_NAME org.kde.pim.ews.client
    DESCRIPTION "ews client (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME
    )
ecm_qt_declare_logging_category(EWSCLIENT_SRCS
    HEADER ewscli_proto_debug.h
    IDENTIFIER EWSCLI_PROTO_LOG
    CATEGORY_NAME org.kde.pim.ews.client.proto
    DEFAULT_SEVERITY Warning
    DESCRIPTION "ews client proto (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME
    )
ecm_qt_declare_logging_category(EWSCLIENT_SRCS
    HEADER ewscli_req_debug.h
    IDENTIFIER EWSCLI_REQUEST_LOG
    CATEGORY_NAME org.kde.pim.ews.client.request
    DEFAULT_SEVERITY Warning
    DESCRIPTION "ews client request (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME
    )
ecm_qt_declare_logging_category(EWSCLIENT_SRCS
    HEADER ewscli_failedreq_debug.h
    IDENTIFIER EWSCLI_FAILEDREQUEST_LOG
    CATEGORY_NAME org.kde.pim.ews.client.failedrequest
    DEFAULT_SEVERITY Warning
    DESCRIPTION "ews client failed request (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME
    )


add_library(ewsclient STATIC ${EWSCLIENT_SRCS})
target_link_libraries(ewsclient
    Qt${QT_MAJOR_VERSION}::Network
    KF5::KIOCore
    KF5::KIOFileWidgets
    KF5::KIOWidgets
    KF5::Codecs
    KF5::I18n
    KF5::Mime
    KF5::CalendarCore)

if (TARGET Qt${QT_MAJOR_VERSION}::NetworkAuth)
    target_link_libraries(ewsclient
        Qt${QT_MAJOR_VERSION}::NetworkAuth
        Qt${QT_MAJOR_VERSION}::WebEngineWidgets)
endif ()

if (Qca-qt5_FOUND)
    target_link_libraries(ewsclient
        qca-qt5)
endif ()
