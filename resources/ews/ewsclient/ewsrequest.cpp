/*
    SPDX-FileCopyrightText: 2015-2019 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsrequest.h"

#include <QNetworkReply>
#include <QTemporaryFile>

#include "auth/ewsabstractauth.h"
#include "ewsclient_debug.h"
#include "transferjob.h"

EwsRequest::EwsRequest(EwsClient &client, QObject *parent)
    : EwsJob(parent)
    , mClient(client)
    , mServerVersion(EwsServerVersion::ewsVersion2007Sp1)
{
}

EwsRequest::~EwsRequest() = default;

void EwsRequest::doSend()
{
    const auto jobs{subjobs()};
    for (KJob *job : jobs) {
        job->start();
    }
}

void EwsRequest::startSoapDocument(QXmlStreamWriter &writer)
{
    writer.writeStartDocument();

    writer.writeNamespace(soapEnvNsUri, QStringLiteral("soap"));
    writer.writeNamespace(ewsMsgNsUri, QStringLiteral("m"));
    writer.writeNamespace(ewsTypeNsUri, QStringLiteral("t"));

    // SOAP Envelope
    writer.writeStartElement(soapEnvNsUri, QStringLiteral("Envelope"));

    // SOAP Header
    writer.writeStartElement(soapEnvNsUri, QStringLiteral("Header"));
    mServerVersion.writeRequestServerVersion(writer);
    writer.writeEndElement();

    // SOAP Body
    writer.writeStartElement(soapEnvNsUri, QStringLiteral("Body"));
}

void EwsRequest::endSoapDocument(QXmlStreamWriter &writer)
{
    // End SOAP Body
    writer.writeEndElement();

    // End SOAP Envelope
    writer.writeEndElement();

    writer.writeEndDocument();
}

void EwsRequest::prepare(const QString &body)
{
    mBody = body;

    QString username, password;
    QHash<QByteArray, QByteArray> customHeaders;
    if (mClient.auth()) {
        if (!mClient.auth()->getAuthData(username, password, customHeaders)) {
            setErrorMsg(QStringLiteral("Failed to retrieve authentication data"));
            Q_EMIT emitResult();
            return;
        }
    }

    QUrl url = mClient.url();
    url.setUserName(username);
    url.setPassword(password);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("text/xml"));
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

    if (!mClient.userAgent().isEmpty()) {
        request.setHeader(QNetworkRequest::UserAgentHeader, mClient.userAgent());
    }

    if (!customHeaders.isEmpty()) {
        for (const auto &[key, header] : customHeaders.asKeyValueRange()) {
            request.setRawHeader(key, header);
        }
    }

    auto job = new TransferJob(request, body.toUtf8());
    if (mClient.isNTLMv2Enabled()) {
        job->setNTLM(username, password);
    }
    connect(job, &KIO::TransferJob::result, this, &EwsRequest::requestResult);
    connect(job, &KIO::TransferJob::percentChanged, this, [this](KJob *job, unsigned long) {
        requestProgress(job);
    });
    addSubjob(job);
}

void EwsRequest::requestProgress(KJob *)
{
}

void EwsRequest::requestResult(KJob *job)
{
    auto trJob = qobject_cast<TransferJob *>(job);
    Q_ASSERT(trJob);

    auto reply = trJob->reply();
    Q_ASSERT(reply);

    mResponseData = trJob->reply()->readAll();

    if (EWSCLI_PROTO_LOG().isDebugEnabled()) {
        ewsLogDir.setAutoRemove(false);
        if (ewsLogDir.isValid()) {
            if (!mResponseData.isEmpty()) {
                QTemporaryFile dumpFile(ewsLogDir.path() + QStringLiteral("/ews_xmldump_XXXXXXX.xml"));
                dumpFile.open();
                dumpFile.setAutoRemove(false);
                dumpFile.write(mResponseData);
                qCDebug(EWSCLI_PROTO_LOG) << "response dumped to" << dumpFile.fileName();
                dumpFile.close();
            } else {
                qCDebug(EWSCLI_PROTO_LOG) << "response is empty";
            }
        }
    }

    const int resp = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt();

    if (resp == 401 && mClient.auth()) {
        mClient.auth()->notifyRequestAuthFailed();
        setEwsResponseCode(EwsResponseCodeUnauthorized);
    }

    if (reply->error() != QNetworkReply::NoError) {
        setErrorMsg(QStringLiteral("Failed to process EWS request: ") + reply->errorString(), reply->error());
    }
    /* Don't attempt to parse the response in case of a HTTP error. The only exception is
     * 500 (Bad Request) as in such case the server does provide the usual SOAP response. */
    else if ((resp >= 300) && (resp != 500)) {
        setErrorMsg(QStringLiteral("Failed to process EWS request - HTTP code %1").arg(resp));
        setError(resp);
    } else {
        QXmlStreamReader reader(mResponseData);
        readResponse(reader);
    }

    emitResult();
}

bool EwsRequest::readResponse(QXmlStreamReader &reader)
{
    if (!reader.readNextStartElement()) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request XML"));
    }

    if ((reader.name() != QLatin1StringView("Envelope")) || (reader.namespaceUri() != soapEnvNsUri)) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - not a SOAP XML"));
    }

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != soapEnvNsUri) {
            return setErrorMsg(QStringLiteral("Failed to read EWS request - not a SOAP XML"));
        }

        if (reader.name() == QLatin1StringView("Body")) {
            if (!readSoapBody(reader)) {
                return false;
            }
        } else if (reader.name() == QLatin1StringView("Header")) {
            if (!readHeader(reader)) {
                return false;
            }
        }
    }
    return true;
}

bool EwsRequest::readSoapBody(QXmlStreamReader &reader)
{
    while (reader.readNextStartElement()) {
        if ((reader.name() == QLatin1StringView("Fault")) && (reader.namespaceUri() == soapEnvNsUri)) {
            return readSoapFault(reader);
        }

        if (!parseResult(reader)) {
            if (EWSCLI_FAILEDREQUEST_LOG().isDebugEnabled()) {
                dump();
            }
            return false;
        }
    }
    return true;
}
QPair<QStringView, QString> EwsRequest::parseNamespacedString(const QString &str, const QXmlStreamNamespaceDeclarations &namespaces)
{
    const auto tokens = str.split(QLatin1Char(':'));
    switch (tokens.count()) {
    case 1:
        return {QStringView(), str};
    case 2:
        for (const auto &ns : namespaces) {
            if (ns.prefix() == tokens[0]) {
                return {ns.namespaceUri(), tokens[1]};
            }
        }
        /* fall through */
    default:
        return {};
    }
}
EwsResponseCode EwsRequest::parseEwsResponseCode(const QPair<QStringView, QString> &code)

{
    if (code.first == ewsTypeNsUri) {
        return decodeEwsResponseCode(code.second);
    } else {
        return EwsResponseCodeUnknown;
    }
}

bool EwsRequest::readSoapFault(QXmlStreamReader &reader)
{
    QString faultCode;
    QString faultString;
    while (reader.readNextStartElement()) {
        if (reader.name() == QLatin1StringView("faultcode")) {
            const auto rawCode = reader.readElementText();
            const auto parsedCode = parseEwsResponseCode(parseNamespacedString(rawCode, reader.namespaceDeclarations()));
            if (parsedCode != EwsResponseCodeUnknown) {
                setEwsResponseCode(parsedCode);
            }
            faultCode = rawCode;
        } else if (reader.name() == QLatin1StringView("faultstring")) {
            faultString = reader.readElementText();
        }
    }

    qCWarning(EWSCLI_LOG) << "readSoapFault" << faultCode;

    setErrorMsg(faultCode + QStringLiteral(": ") + faultString);

    if (EWSCLI_FAILEDREQUEST_LOG().isDebugEnabled()) {
        dump();
    }

    return false;
}

bool EwsRequest::parseResponseMessage(QXmlStreamReader &reader, const QString &reqName, ContentReaderFn contentReader)
{
    if (reader.name().toString() != reqName + QStringLiteral("Response") || reader.namespaceUri() != ewsMsgNsUri) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected %1 element.").arg(reqName + QStringLiteral("Response")));
    }

    if (!reader.readNextStartElement()) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected a child element in %1 element.").arg(reqName + QStringLiteral("Response")));
    }

    if (reader.name().toString() != QLatin1StringView("ResponseMessages") || reader.namespaceUri() != ewsMsgNsUri) {
        return setErrorMsg(QStringLiteral("Failed to read EWS request - expected %1 element.").arg(QStringLiteral("ResponseMessages")));
    }

    while (reader.readNextStartElement()) {
        if (reader.name().toString() != reqName + QStringLiteral("ResponseMessage") || reader.namespaceUri() != ewsMsgNsUri) {
            return setErrorMsg(QStringLiteral("Failed to read EWS request - expected %1 element.").arg(reqName + QStringLiteral("ResponseMessage")));
        }

        if (!contentReader(reader)) {
            return false;
        }
    }

    return true;
}

void EwsRequest::setServerVersion(const EwsServerVersion &version)
{
    mServerVersion = version;
}

EwsRequest::Response::Response(QXmlStreamReader &reader)
{
    static constexpr auto respClasses = std::to_array({
        QLatin1StringView("Success"),
        QLatin1StringView("Warning"),
        QLatin1StringView("Error"),
    });

    auto respClassRef = reader.attributes().value(QStringLiteral("ResponseClass"));
    if (respClassRef.isNull()) {
        mClass = EwsResponseParseError;
        qCWarning(EWSCLI_LOG) << "ResponseClass attribute not found in response element";
        return;
    }

    unsigned i = 0;
    for (const auto &respClass : respClasses) {
        if (respClass == respClassRef) {
            mClass = static_cast<EwsResponseClass>(i);
            break;
        }
        i++;
    }
}

bool EwsRequest::Response::readResponseElement(QXmlStreamReader &reader)
{
    if (reader.namespaceUri() != ewsMsgNsUri) {
        return false;
    }
    if (reader.name() == QLatin1StringView("ResponseCode")) {
        mCode = reader.readElementText();
    } else if (reader.name() == QLatin1StringView("MessageText")) {
        mMessage = reader.readElementText();
    } else if (reader.name() == QLatin1StringView("DescriptiveLinkKey")) {
        reader.skipCurrentElement();
    } else if (reader.name() == QLatin1StringView("MessageXml")) {
        reader.skipCurrentElement();
    } else if (reader.name() == QLatin1StringView("ErrorSubscriptionIds")) {
        reader.skipCurrentElement();
    } else {
        return false;
    }
    return true;
}

bool EwsRequest::readHeader(QXmlStreamReader &reader)
{
    while (reader.readNextStartElement()) {
        if (reader.name() == QLatin1StringView("ServerVersionInfo") && reader.namespaceUri() == ewsTypeNsUri) {
            EwsServerVersion version(reader);
            if (!version.isValid()) {
                qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read EWS request - error parsing server version.");
                return false;
            }
            mServerVersion = version;
            mClient.setServerVersion(version);
            reader.skipCurrentElement();
        } else {
            reader.skipCurrentElement();
        }
    }

    return true;
}

bool EwsRequest::Response::setErrorMsg(const QString &msg)
{
    mClass = EwsResponseParseError;
    mCode = QStringLiteral("ResponseParseError");
    mMessage = msg;
    qCWarningNC(EWSCLI_LOG) << msg;
    return false;
}

void EwsRequest::dump() const
{
    ewsLogDir.setAutoRemove(false);
    if (ewsLogDir.isValid()) {
        QTemporaryFile reqDumpFile(ewsLogDir.path() + QStringLiteral("/ews_xmlreqdump_XXXXXXX.xml"));
        reqDumpFile.open();
        reqDumpFile.setAutoRemove(false);
        reqDumpFile.write(mBody.toUtf8());
        reqDumpFile.close();
        QTemporaryFile resDumpFile(ewsLogDir.path() + QStringLiteral("/ews_xmlresdump_XXXXXXX.xml"));
        resDumpFile.open();
        resDumpFile.setAutoRemove(false);
        resDumpFile.write(mResponseData);
        resDumpFile.close();
        qCDebug(EWSCLI_LOG) << "request  dumped to" << reqDumpFile.fileName();
        qCDebug(EWSCLI_LOG) << "response dumped to" << resDumpFile.fileName();
    } else {
        qCWarning(EWSCLI_LOG) << "failed to dump request and response";
    }
}

#include "moc_ewsrequest.cpp"
