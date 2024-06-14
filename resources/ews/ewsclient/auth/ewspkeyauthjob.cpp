/*
    SPDX-FileCopyrightText: 2018 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewspkeyauthjob.h"

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslKey>
#include <QUrlQuery>

#include <gcrypt.h>

static const QMap<QByteArray, QSslCertificate::SubjectInfo> stringToKnownCertInfoType = {
    {QByteArray("CN"), QSslCertificate::CommonName},
    {QByteArray("L"), QSslCertificate::LocalityName},
    {QByteArray("ST"), QSslCertificate::StateOrProvinceName},
    {QByteArray("O"), QSslCertificate::Organization},
    {QByteArray("OU"), QSslCertificate::OrganizationalUnitName},
    {QByteArray("C"), QSslCertificate::CountryName},
    {QByteArray("emailAddress"), QSslCertificate::EmailAddress},
};

static QHash<QSslCertificate::SubjectInfo, QStringList> parseCertSubjectInfo(const QString &info)
{
    QHash<QSslCertificate::SubjectInfo, QStringList> map;
    const auto infos{info.split(QLatin1Char(','), Qt::SkipEmptyParts)};
    for (const auto &token : infos) {
        const auto keyval = token.trimmed().split(QLatin1Char('='));
        if (keyval.count() == 2) {
            const auto attr = keyval[0].toUtf8();
            if (stringToKnownCertInfoType.contains(attr)) {
                map[stringToKnownCertInfoType[attr]].append(keyval[1]);
            }
        }
    }

    return map;
}

static QString escapeSlashes(const QString &str)
{
    QString result = str;
    return result.replace(QLatin1Char('/'), QStringLiteral("\\/"));
}

EwsPKeyAuthJob::EwsPKeyAuthJob(const QUrl &pkeyUri, const QString &certFile, const QString &keyFile, const QString &keyPassword, QObject *parent)
    : EwsJob(parent)
    , mPKeyUri(pkeyUri)
    , mCertFile(certFile)
    , mKeyFile(keyFile)
    , mKeyPassword(keyPassword)
    , mNetworkAccessManager(new QNetworkAccessManager(this))
{
}

EwsPKeyAuthJob::~EwsPKeyAuthJob()
{
}

void EwsPKeyAuthJob::start()
{
    const QUrlQuery query(mPKeyUri);
    QMap<QString, QString> params;
    for (const auto &it : query.queryItems()) {
        params[it.first.toLower()] = QUrl::fromPercentEncoding(it.second.toLatin1());
    }

    if (params.contains(QStringLiteral("submiturl")) && params.contains(QStringLiteral("nonce")) && params.contains(QStringLiteral("certauthorities"))
        && params.contains(QStringLiteral("context")) && params.contains(QStringLiteral("version"))) {
        const auto respToken = buildAuthResponse(params);

        if (!respToken.isEmpty()) {
            sendAuthRequest(respToken, QUrl(params[QStringLiteral("submiturl")]), params[QStringLiteral("context")]);
        } else {
            emitResult();
        }
    } else {
        setErrorMsg(QStringLiteral("Missing one or more input parameters"));
        emitResult();
    }
}

void EwsPKeyAuthJob::sendAuthRequest(const QByteArray &respToken, const QUrl &submitUrl, const QString &context)
{
    QNetworkRequest req(submitUrl);

    req.setRawHeader("Authorization",
                     QStringLiteral("PKeyAuth AuthToken=\"%1\",Context=\"%2\",Version=\"1.0\"").arg(QString::fromLatin1(respToken), context).toLatin1());

    mAuthReply.reset(mNetworkAccessManager->get(req));

    connect(mAuthReply.data(), &QNetworkReply::finished, this, &EwsPKeyAuthJob::authRequestFinished);
}

void EwsPKeyAuthJob::authRequestFinished()
{
    if (mAuthReply->error() == QNetworkReply::NoError) {
        mResultUri = mAuthReply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (!mResultUri.isValid()) {
            setErrorMsg(QStringLiteral("Incorrect or missing redirect URI in PKeyAuth response"));
        }
    } else {
        setErrorMsg(QStringLiteral("Failed to process PKeyAuth request: %1").arg(mAuthReply->errorString()));
    }
    emitResult();
}

QByteArray EwsPKeyAuthJob::buildAuthResponse(const QMap<QString, QString> &params)
{
    if (params[QStringLiteral("version")] != QLatin1StringView("1.0")) {
        setErrorMsg(QStringLiteral("Unknown version of PKey Authentication: %1").arg(params[QStringLiteral("version")]));
        return QByteArray();
    }

    const auto authoritiesInfo = parseCertSubjectInfo(params[QStringLiteral("certauthorities")]);

    const auto certificates = QSslCertificate::fromPath(mCertFile, QSsl::Pem);

    if (certificates.isEmpty()) {
        setErrorMsg(QStringLiteral("Certificate import failed"));
        return QByteArray();
    }

    QSslCertificate cert;
    for (const auto &c : certificates) {
        const auto issuerInfoAttributes = c.issuerInfoAttributes();
        bool match = true;
        for (const auto &attribute : issuerInfoAttributes) {
            if (!stringToKnownCertInfoType.contains(attribute) || c.issuerInfo(attribute) != authoritiesInfo[stringToKnownCertInfoType[attribute]]) {
                match = false;
                break;
            }
        }
        if (match) {
            cert = c;
            break;
        }
    }

    if (cert.isNull()) {
        setErrorMsg(QStringLiteral("No suitable certificate found"));
        return QByteArray();
    }

    QFile keyFile(mKeyFile);
    if (!keyFile.open(QIODeviceBase::ReadOnly)) {
        setErrorMsg(QStringLiteral("Private key import failed. File not found or not readable."));
        return QByteArray();
    }

    gcry_check_version(NULL);
    gcry_control(GCRYCTL_ENABLE_M_GUARD);
    gcry_control(GCRYCTL_DISABLE_SECMEM);
    gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

    gcry_sexp_t rsa_private;

    QSslKey privateKey(keyFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey, mKeyPassword.toUtf8());

    // QCA::PrivateKey privateKey = QCA::PrivateKey::fromPEMFile(mKeyFile, mKeyPassword.toUtf8(), &importResult);
    if (privateKey.isNull()) {
        setErrorMsg(QStringLiteral("Private key import failed"));
        return QByteArray();
    }

    const QString certStr = escapeSlashes(QString::fromLatin1(cert.toDer().toBase64()));
    const QString header = QStringLiteral("{\"x5c\":[\"%1\"],\"typ\":\"JWT\",\"alg\":\"RS256\"}").arg(certStr);

    const QString payload = QStringLiteral("{\"nonce\":\"%1\",\"iat\":\"%2\",\"aud\":\"%3\"}")
                                .arg(params[QStringLiteral("nonce")])
                                .arg(QDateTime::currentSecsSinceEpoch())
                                .arg(escapeSlashes(params[QStringLiteral("submiturl")]));

    const auto headerB64 = header.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    const auto payloadB64 = payload.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    QCA::SecureArray data(headerB64 + '.' + payloadB64);

    QByteArray sig = privateKey.signMessage(data, QCA::EMSA3_SHA256, QCA::IEEE_1363);

    return headerB64 + '.' + payloadB64 + '.' + sig.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

const QUrl &EwsPKeyAuthJob::resultUri() const
{
    return mResultUri;
}

QString EwsPKeyAuthJob::getAuthHeader()
{
    const QUrlQuery query(mPKeyUri);
    QMap<QString, QString> params;
    for (const auto &it : query.queryItems()) {
        params[it.first.toLower()] = QUrl::fromPercentEncoding(it.second.toLatin1());
    }

    if (params.contains(QStringLiteral("submiturl")) && params.contains(QStringLiteral("nonce")) && params.contains(QStringLiteral("certauthorities"))
        && params.contains(QStringLiteral("context")) && params.contains(QStringLiteral("version"))) {
        const auto respToken = buildAuthResponse(params);

        if (!respToken.isEmpty()) {
            return QLatin1StringView("PKeyAuth AuthToken=\"%1\",Context=\"%2\",Version=\"1.0\"")
                .arg(QString::fromLatin1(respToken), params[QStringLiteral("context")]);
        } else {
            return {};
        }
    } else {
        return {};
    }
}

#include "moc_ewspkeyauthjob.cpp"
