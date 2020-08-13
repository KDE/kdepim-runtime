/*
    SPDX-FileCopyrightText: 2016 Stefan Stäglich <sstaeglich@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef TOMBOYJOBBASE_H
#define TOMBOYJOBBASE_H

#include "o1tomboy.h"
#include "o2/o1requestor.h"
#include <KCompositeJob>
#include <KIO/AccessManager>
#include <QString>

enum TomboyJobError {
    NoError,
    TemporaryError,
    PermanentError
};

class TomboyJobBase : public KCompositeJob
{
    Q_OBJECT
public:
    explicit TomboyJobBase(KIO::AccessManager *manager, QObject *parent = nullptr);

    void setServerURL(const QString &apiurl, const QString &contenturl);
    void setAuthentication(const QString &token, const QString &secret);

protected:
    KIO::Integration::AccessManager *mManager = nullptr;
    O1Requestor *mRequestor = nullptr;
    O1Tomboy *mO1 = nullptr;
    QNetworkReply *mReply = nullptr;

    QString mApiURL;
    QString mContentURL;

    void checkReplyError();
};

#endif // TOMBOYJOBBASE_H
