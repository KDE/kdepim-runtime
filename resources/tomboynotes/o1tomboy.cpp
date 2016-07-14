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

#include "o1tomboy.h"

O1Tomboy::O1Tomboy(QObject *parent) : O1(parent)
{
}

void O1Tomboy::setBaseURL(const QString &value)
{
    setRequestTokenUrl(QUrl(value + QStringLiteral("/oauth/request_token")));
    setAuthorizeUrl(QUrl(value + QStringLiteral("/oauth/authorize")));
    setAccessTokenUrl(QUrl(value + QStringLiteral("/oauth/access_token")));
    setClientId("anyone");
    setClientSecret("anyone");
}

QString O1Tomboy::getRequestToken() const
{
    return requestToken_;
}

QString O1Tomboy::getRequestTokenSecret() const
{
    return requestTokenSecret_;
}

void O1Tomboy::restoreAuthData(const QString &token, const QString &secret)
{
    requestToken_ = token;
    requestTokenSecret_ = secret;
    setLinked(true);
}
