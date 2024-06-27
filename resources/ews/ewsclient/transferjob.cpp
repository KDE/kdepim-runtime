// SPDX-FileCopyrightText: 2024 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "transferjob.h"
#include <QAuthenticator>
#include <QNetworkAccessManager>
#include <QNetworkReply>

Q_GLOBAL_STATIC(QNetworkAccessManager, qnam)

TransferJob::TransferJob(const QNetworkRequest &request, const QByteArray &body)
    : KJob()
    , mRequest(request)
    , mBody(body)
{
}

TransferJob::~TransferJob() = default;

void TransferJob::start()
{
    mReply.reset(qnam->post(mRequest, mBody));
    connect(mReply.get(), &QNetworkReply::finished, this, [this] {
        emitResult();
    });
    connect(mReply.get(), &QNetworkReply::downloadProgress, this, [this](qint64 bytesReceived, qint64 bytesTotal) {
        Q_UNUSED(bytesTotal);
        setProcessedAmount(Unit::Bytes, bytesReceived);
    });

    connect(qnam, &QNetworkAccessManager::authenticationRequired, this, [this](QNetworkReply *reply, QAuthenticator *authenticator) {
        if (mReply.get() != reply) {
            return;
        }
        if (!mUsername.isEmpty() && !mPassword.isEmpty()) {
            authenticator->setPassword(mPassword);
            authenticator->setUser(mUsername);
        }
    });
}

QNetworkReply *TransferJob::reply() const
{
    return mReply.get();
}

void TransferJob::setNTLM(const QString &username, const QString &password)
{
    mUsername = username;
    mPassword = password;
}
