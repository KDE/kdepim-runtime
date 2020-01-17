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

#include "tomboyjobbase.h"

TomboyJobBase::TomboyJobBase(KIO::AccessManager *manager, QObject *parent)
    : KCompositeJob(parent)
    , mManager(manager)
    , mO1(new O1Tomboy(this))
    , mReply(nullptr)
{
    mRequestor = new O1Requestor(mManager, mO1, this);
}

void TomboyJobBase::setServerURL(const QString &apiurl, const QString &contenturl)
{
    mO1->setBaseURL(apiurl);
    mApiURL = apiurl + QStringLiteral("/api/1.0");
    mContentURL = contenturl;
}

void TomboyJobBase::setAuthentication(const QString &token, const QString &secret)
{
    mO1->restoreAuthData(token, secret);
}

void TomboyJobBase::checkReplyError()
{
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
    const auto networkError = mReply->error();
#else
    const auto networkError = mReply->networkError();
#endif
    switch (networkError) {
    case QNetworkReply::NoError:
        setError(TomboyJobError::NoError);
        break;
    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::TimeoutError:
        setError(TomboyJobError::TemporaryError);
        break;
    default:
        setError(TomboyJobError::PermanentError);
        break;
    }
}
