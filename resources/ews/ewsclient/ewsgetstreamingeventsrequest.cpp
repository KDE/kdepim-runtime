/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsgetstreamingeventsrequest.h"

#include <QTemporaryFile>

#include "ewsclient_debug.h"

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

static constexpr auto respChunkTimeout = 250ms;

EwsGetStreamingEventsRequest::EwsGetStreamingEventsRequest(EwsClient &client, QObject *parent)
    : EwsEventRequestBase(client, QStringLiteral("GetStreamingEvents"), parent)
    , mTimeout(30s)
    , mRespTimer(this)
{
    mRespTimer.setInterval(respChunkTimeout);
    connect(&mRespTimer, &QTimer::timeout, this, &EwsGetStreamingEventsRequest::requestDataTimeout);
}

EwsGetStreamingEventsRequest::~EwsGetStreamingEventsRequest() = default;

void EwsGetStreamingEventsRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    if (!serverVersion().supports(EwsServerVersion::StreamingSubscription)) {
        setServerVersion(EwsServerVersion::minSupporting(EwsServerVersion::StreamingSubscription));
    }

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, "GetStreamingEvents"_L1);

    writer.writeStartElement(ewsMsgNsUri, "SubscriptionIds"_L1);
    writer.writeTextElement(ewsTypeNsUri, "SubscriptionId"_L1, mSubscriptionId);
    writer.writeEndElement();

    writer.writeTextElement(ewsMsgNsUri, "ConnectionTimeout"_L1, QString::number(mTimeout.count()));

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebugNC(EWSCLI_REQUEST_LOG)
        << QStringLiteral("Starting GetStreamingEvents request (subId: %1, timeout: %2)").arg(ewsHash(mSubscriptionId)).arg(mTimeout.count());

    qCDebug(EWSCLI_PROTO_LOG) << reqString;

    prepare(reqString);

    doSend();
}

void EwsGetStreamingEventsRequest::requestProgress(KJob *job)
{
    Q_UNUSED(job)

    mRespTimer.stop();
    qCDebug(EWSCLI_PROTO_LOG) << "progress" << job;
    mRespTimer.start();
}

void EwsGetStreamingEventsRequest::requestDataTimeout()
{
    if (mResponseData.isEmpty()) {
        return;
    }
    if (EWSCLI_PROTO_LOG().isDebugEnabled()) {
        ewsLogDir.setAutoRemove(false);
        if (ewsLogDir.isValid()) {
            QTemporaryFile dumpFile(ewsLogDir.path() + QStringLiteral("/ews_xmldump_XXXXXXX.xml"));
            dumpFile.open();
            dumpFile.setAutoRemove(false);
            dumpFile.write(mResponseData);
            qCDebug(EWSCLI_PROTO_LOG) << "response dumped to" << dumpFile.fileName();
            dumpFile.close();
        }
    }

    QXmlStreamReader reader(mResponseData);
    if (!readResponse(reader)) {
        const auto jobs{subjobs()};
        for (KJob *job : jobs) {
            removeSubjob(job);
            job->kill();
        }
        emitResult();
    } else {
        Q_EMIT eventsReceived(this);
    }

    mResponseData.clear();
}

void EwsGetStreamingEventsRequest::eventsProcessed(const Response &resp)
{
    mResponses.removeOne(resp);
}

#include "moc_ewsgetstreamingeventsrequest.cpp"
