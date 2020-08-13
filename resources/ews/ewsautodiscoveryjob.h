/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EWSAUTODISCOVERYJOB_H
#define EWSAUTODISCOVERYJOB_H

#include <QQueue>

#include "ewsjob.h"

class EwsAutodiscoveryJob : public EwsJob
{
    Q_OBJECT
public:
    EwsAutodiscoveryJob(const QString &email, const QString &username, const QString &password, const QString &userAgent, bool enableNTLMv2, QObject *parent);
    ~EwsAutodiscoveryJob() override;

    void start() override;

    const QString &ewsUrl() const
    {
        return mEwsUrl;
    }

    const QString &oabUrl() const
    {
        return mOabUrl;
    }

private Q_SLOTS:
    void autodiscoveryRequestFinished(KJob *job);
private:
    void addUrls(const QString &domain);
    void sendNextRequest(bool useCreds);
    void parseEmail();

    QString mEmail;
    QString mUsername;
    QString mPassword;

    QString mUserAgent;
    bool mEnableNTLMv2;

    QQueue<QString> mUrlQueue;

    QString mEwsUrl;
    QString mOabUrl;
    bool mUsedCreds;
};

#endif
