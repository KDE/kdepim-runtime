/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsgeteventsrequest.h"
#include "ewsclient_debug.h"

using namespace Qt::StringLiterals;

EwsGetEventsRequest::EwsGetEventsRequest(EwsClient &client, QObject *parent)
    : EwsEventRequestBase(client, "GetEvents"_L1, parent)
{
}

EwsGetEventsRequest::~EwsGetEventsRequest() = default;

void EwsGetEventsRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, "GetEvents"_L1);

    writer.writeTextElement(ewsMsgNsUri, "SubscriptionId"_L1, mSubscriptionId);

    writer.writeTextElement(ewsMsgNsUri, "Watermark"_L1, mWatermark);

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebugNC(EWSCLI_REQUEST_LOG) << QStringLiteral("Starting GetEvents request (subId: %1, wmark: %2)").arg(mSubscriptionId, mWatermark);

    qCDebug(EWSCLI_PROTO_LOG) << reqString;

    prepare(reqString);

    doSend();
}

#include "moc_ewsgeteventsrequest.cpp"
