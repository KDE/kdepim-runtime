/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsgetstreamingeventsrequest.h"

#include <QTemporaryFile>

#include <cctype>

#include "ewsclient_debug.h"

using namespace std::chrono_literals;
using namespace Qt::Literals::StringLiterals;

EwsGetStreamingEventsRequest::EwsGetStreamingEventsRequest(EwsClient &client, QObject *parent)
    : EwsEventRequestBase(client, QStringLiteral("GetStreamingEvents"), parent)
    , mTimeout(30min)
{
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

void EwsGetStreamingEventsRequest::requestDataReceived()
{
    /* The server keeps the connection open and sends each notification as a
     * separate SOAP document. Data may arrive in arbitrary pieces: parse every
     * document that is complete, and keep the incomplete rest for later. */
    bool parsed = false;
    while (true) {
        // An XML declaration must be at the start of a document.
        qsizetype start = 0;
        while (start < mResponseData.size() && std::isspace(static_cast<unsigned char>(mResponseData.at(start)))) {
            ++start;
        }
        mResponseData.remove(0, start);
        if (mResponseData.isEmpty()) {
            break;
        }

        const qsizetype size = completeDocumentSize();
        if (size == 0) {
            break;
        }

        // On malformed XML, parse all data to fail with the parser's error.
        const QByteArray document = size > 0 ? mResponseData.left(size) : mResponseData;
        dumpDocument(document);
        QXmlStreamReader reader(document);
        if (!readResponse(reader)) {
            const auto jobs{subjobs()};
            for (KJob *job : jobs) {
                removeSubjob(job);
                job->kill();
            }
            emitResult();
            return;
        }
        mResponseData.remove(0, document.size());
        mResponseDataParsed = true;
        parsed = true;
    }

    if (parsed) {
        Q_EMIT eventsReceived(this);
    }
}

qsizetype EwsGetStreamingEventsRequest::completeDocumentSize() const
{
    /* Find the end of the root element. The reader counts characters, so
     * parse the decoded text and convert the offset back to bytes. */
    const QString text = QString::fromUtf8(mResponseData);
    QXmlStreamReader reader(text);
    int depth = 0;
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement:
            ++depth;
            break;
        case QXmlStreamReader::EndElement:
            if (--depth == 0) {
                return QStringView(text).left(reader.characterOffset()).toUtf8().size();
            }
            break;
        default:
            break;
        }
    }
    return reader.error() == QXmlStreamReader::PrematureEndOfDocumentError ? 0 : -1;
}

void EwsGetStreamingEventsRequest::dumpDocument(const QByteArray &document) const
{
    if (!EWSCLI_PROTO_LOG().isDebugEnabled()) {
        return;
    }
    ewsLogDir.setAutoRemove(false);
    if (ewsLogDir.isValid()) {
        QTemporaryFile dumpFile(ewsLogDir.path() + QStringLiteral("/ews_xmldump_XXXXXXX.xml"));
        if (dumpFile.open()) {
            dumpFile.setAutoRemove(false);
            dumpFile.write(document);
            qCDebug(EWSCLI_PROTO_LOG) << "response dumped to" << dumpFile.fileName();
            dumpFile.close();
        } else {
            qCWarning(EWSCLI_PROTO_LOG) << "failed to dump response:" << dumpFile.errorString();
        }
    }
}

void EwsGetStreamingEventsRequest::eventsProcessed(const Response &resp)
{
    mResponses.removeOne(resp);
}

#include "moc_ewsgetstreamingeventsrequest.cpp"
