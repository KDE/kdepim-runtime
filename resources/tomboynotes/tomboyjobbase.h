/*
    Copyright (c) 2016 Stefan Stäglich <sstaeglich@kdemail.net>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
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
