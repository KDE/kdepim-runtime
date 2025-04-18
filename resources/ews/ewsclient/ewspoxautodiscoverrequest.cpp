/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewspoxautodiscoverrequest.h"

#include <QTemporaryFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <KIO/TransferJob>

#include "ewsclient_debug.h"

using namespace Qt::StringLiterals;

static constexpr QLatin1StringView poxAdOuReqNsUri = "http://schemas.microsoft.com/exchange/autodiscover/outlook/requestschema/2006"_L1;
static constexpr QLatin1StringView poxAdRespNsUri = "http://schemas.microsoft.com/exchange/autodiscover/responseschema/2006"_L1;
static constexpr QLatin1StringView poxAdOuRespNsUri = "http://schemas.microsoft.com/exchange/autodiscover/outlook/responseschema/2006a"_L1;

EwsPoxAutodiscoverRequest::EwsPoxAutodiscoverRequest(const QUrl &url, const QString &email, const QString &userAgent, bool useNTLMv2, QObject *parent)
    : EwsJob(parent)
    , mUrl(url)
    , mEmail(email)
    , mUserAgent(userAgent)
    , mUseNTLMv2(useNTLMv2)
    , mServerVersion(EwsServerVersion::ewsVersion2007Sp1)
    , mAction(Settings)
{
}

EwsPoxAutodiscoverRequest::~EwsPoxAutodiscoverRequest() = default;

void EwsPoxAutodiscoverRequest::doSend()
{
    const auto jobs{subjobs()};
    for (KJob *job : jobs) {
        job->start();
    }
}

void EwsPoxAutodiscoverRequest::prepare(const QString &body)
{
    mBody = body;
    mLastUrl = mUrl;
    KIO::TransferJob *job = KIO::http_post(mUrl, body.toUtf8(), KIO::HideProgressInfo);
    job->addMetaData(QStringLiteral("content-type"), QStringLiteral("text/xml"));
    job->addMetaData(QStringLiteral("no-auth-prompt"), QStringLiteral("true"));
    job->addMetaData(QStringLiteral("HttpVersion"), QStringLiteral("http1"));
    if (mUseNTLMv2) {
        job->addMetaData(QStringLiteral("EnableNTLMv2Auth"), QStringLiteral("true"));
    }
    if (!mUserAgent.isEmpty()) {
        job->addMetaData(QStringLiteral("UserAgent"), mUserAgent);
    }
    // config->readEntry("no-spoof-check", false)

    connect(job, &KIO::TransferJob::result, this, &EwsPoxAutodiscoverRequest::requestResult);
    connect(job, &KIO::TransferJob::data, this, &EwsPoxAutodiscoverRequest::requestData);
    connect(job, &KIO::TransferJob::redirection, this, &EwsPoxAutodiscoverRequest::requestRedirect);

    addSubjob(job);
}

void EwsPoxAutodiscoverRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    writer.writeStartDocument();

    writer.writeDefaultNamespace(poxAdOuReqNsUri);

    writer.writeStartElement(poxAdOuReqNsUri, "Autodiscover"_L1);

    writer.writeStartElement(poxAdOuReqNsUri, "Request"_L1);

    writer.writeTextElement(poxAdOuReqNsUri, "EMailAddress"_L1, mEmail);
    writer.writeTextElement(poxAdOuReqNsUri, "AcceptableResponseSchema"_L1, poxAdOuRespNsUri);

    writer.writeEndElement(); // Request

    writer.writeEndElement(); // Autodiscover

    writer.writeEndDocument();

    qCDebug(EWSCLI_PROTO_LOG) << reqString;

    qCDebugNC(EWSCLI_REQUEST_LOG) << QStringLiteral("Starting POX Autodiscovery request (url: ") << mUrl << QStringLiteral(", email: ") << mEmail;
    prepare(reqString);

    doSend();
}

void EwsPoxAutodiscoverRequest::requestData(KIO::Job *job, const QByteArray &data)
{
    Q_UNUSED(job)

    qCDebug(EWSCLI_PROTO_LOG) << "data" << job << data;
    mResponseData += QString::fromUtf8(data);
}

void EwsPoxAutodiscoverRequest::requestResult(KJob *job)
{
    if (EWSCLI_PROTO_LOG().isDebugEnabled()) {
        ewsLogDir.setAutoRemove(false);
        if (ewsLogDir.isValid()) {
            QTemporaryFile dumpFile(ewsLogDir.path() + QStringLiteral("/ews_xmldump_XXXXXXX.xml"));
            dumpFile.open();
            dumpFile.setAutoRemove(false);
            dumpFile.write(mResponseData.toUtf8());
            qCDebug(EWSCLI_PROTO_LOG) << "response dumped to" << dumpFile.fileName();
            dumpFile.close();
        }
    }

    auto trJob = qobject_cast<KIO::TransferJob *>(job);
    int resp = trJob->metaData()["responsecode"_L1].toUInt();

    if (job->error() != KJob::NoError) {
        setErrorMsg(QStringLiteral("Failed to process EWS request: ") + job->errorString());
        setError(job->error());
    } else if (resp >= 300) {
        setErrorMsg(QStringLiteral("Failed to process EWS request - HTTP code %1").arg(resp));
        setError(resp);
    } else {
        QXmlStreamReader reader(mResponseData);
        readResponse(reader);
    }

    emitResult();
}

bool EwsPoxAutodiscoverRequest::readResponse(QXmlStreamReader &reader)
{
    if (!reader.readNextStartElement()) {
        return setErrorMsg(QStringLiteral("Failed to read POX response XML"));
    }

    if ((reader.name() != "Autodiscover"_L1) || (reader.namespaceUri() != poxAdRespNsUri)) {
        return setErrorMsg(QStringLiteral("Failed to read POX response - not an Autodiscover response"));
    }

    if (!reader.readNextStartElement()) {
        return setErrorMsg(QStringLiteral("Failed to read POX response - expected %1 element").arg(QStringLiteral("Response")));
    }

    if ((reader.name() != "Response"_L1) || (reader.namespaceUri() != poxAdOuRespNsUri)) {
        return setErrorMsg(
            QStringLiteral("Failed to read POX response - expected %1 element, found %2").arg(QStringLiteral("Response").arg(reader.name().toString())));
    }

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != poxAdOuRespNsUri) {
            return setErrorMsg(QStringLiteral("Failed to read POX response - invalid namespace"));
        }

        if (reader.name() == "User"_L1) {
            reader.skipCurrentElement();
        } else if (reader.name() == "Account"_L1) {
            if (!readAccount(reader)) {
                return false;
            }
        } else {
            return setErrorMsg(
                QStringLiteral("Failed to read POX response - unknown element '%1' inside '%2'").arg(reader.name().toString(), QStringLiteral("Response")));
        }
    }
    return true;
}

bool EwsPoxAutodiscoverRequest::readAccount(QXmlStreamReader &reader)
{
    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != poxAdOuRespNsUri) {
            return setErrorMsg(QStringLiteral("Failed to read POX response - invalid namespace"));
        }
        const QStringView readerName = reader.name();
        if (readerName == "Action"_L1) {
            QString action = reader.readElementText();
            if (action == "settings"_L1) {
                mAction = Settings;
            } else if (action == "redirectUrl"_L1) {
                mAction = RedirectUrl;
            } else if (action == "redirectAddr"_L1) {
                mAction = RedirectAddr;
            } else {
                return setErrorMsg(QStringLiteral("Failed to read POX response - unknown action '%1'").arg(action));
            }
        } else if (readerName == "RedirectUrl"_L1) {
            mRedirectUrl = reader.readElementText();
        } else if (readerName == "RedirectAddr"_L1) {
            mRedirectAddr = reader.readElementText();
        } else if (readerName == "RedirectAddr"_L1) {
            mRedirectAddr = reader.readElementText();
        } else if (readerName == "Protocol"_L1) {
            if (!readProtocol(reader)) {
                return false;
            }
        } else {
            reader.skipCurrentElement();
        }
    }
    return true;
}

bool EwsPoxAutodiscoverRequest::readProtocol(QXmlStreamReader &reader)
{
    Protocol proto;

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != poxAdOuRespNsUri) {
            return setErrorMsg(QStringLiteral("Failed to read POX response - invalid namespace"));
        }
        const QStringView readerName = reader.name();
        if (readerName == "Type"_L1) {
            QString type = reader.readElementText();
            if (type == "EXCH"_L1) {
                proto.mType = ExchangeProto;
            } else if (type == "EXPR"_L1) {
                proto.mType = ExchangeProxyProto;
            } else if (type == "WEB"_L1) {
                proto.mType = ExchangeWebProto;
            } else {
                return setErrorMsg(QStringLiteral("Failed to read POX response - unknown protocol '%1'").arg(type));
            }
        } else if (readerName == "EwsUrl"_L1) {
            proto.mEwsUrl = reader.readElementText();
        } else if (readerName == "OabUrl"_L1) {
            proto.mOabUrl = reader.readElementText();
        } else {
            reader.skipCurrentElement();
        }
    }

    qCDebug(EWSCLI_LOG) << "Adding proto type" << proto.mType << proto.isValid();
    mProtocols[proto.mType] = proto;

    return true;
}

void EwsPoxAutodiscoverRequest::requestRedirect(KIO::Job *job, const QUrl &url)
{
    Q_UNUSED(job)

    qCDebugNC(EWSCLI_REQUEST_LOG) << QStringLiteral("Got HTTP redirect to: ") << mUrl;

    mLastUrl = url;
}

void EwsPoxAutodiscoverRequest::dump() const
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
        resDumpFile.write(mResponseData.toUtf8());
        resDumpFile.close();
        qCDebug(EWSCLI_LOG) << "request  dumped to" << reqDumpFile.fileName();
        qCDebug(EWSCLI_LOG) << "response dumped to" << resDumpFile.fileName();
    } else {
        qCWarning(EWSCLI_LOG) << "failed to dump request and response";
    }
}

#include "moc_ewspoxautodiscoverrequest.cpp"
