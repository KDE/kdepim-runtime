// SPDX-FileCopyrightText: 2024 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <KJob>
#include <QNetworkRequest>

#include <memory>

class QNetworkReply;

/// Wrap a QNetworkRequest in a KJob to create POST HTTP request.
class TransferJob : public KJob
{
    Q_OBJECT

public:
    /// Transfer job constructor
    TransferJob(const QNetworkRequest &request, const QByteArray &body);

    void start() override;

    /// Set username and password for a NTLMv2 authentication.
    void setNTLM(const QString &username, const QString &password);

    /// Return the reply or null if the request wasn't sent.
    QNetworkReply *reply() const;

    /// Close the idle connections kept for reuse, e.g., when the network went down.
    static void clearConnectionCache();

Q_SIGNALS:
    /// Emitted for each chunk of the response body as it arrives. The data is
    /// consumed from the reply, so reply()->readAll() only returns what has
    /// not been passed on by this signal yet.
    void dataReceived(const QByteArray &data);

private:
    QNetworkRequest mRequest;
    const QByteArray mBody;
    std::unique_ptr<QNetworkReply> mReply;
    QString mUsername;
    QString mPassword;
};
