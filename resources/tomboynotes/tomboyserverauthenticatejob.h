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

#ifndef TOMBOYSERVERAUTHENTICATEJOB_H
#define TOMBOYSERVERAUTHENTICATEJOB_H

#include "tomboyjobbase.h"
#include <QString>
#include <QWebEngineView>

class TomboyServerAuthenticateJob : public TomboyJobBase
{
    Q_OBJECT
public:
    explicit TomboyServerAuthenticateJob(KIO::AccessManager *manager, QObject *parent = nullptr);

    ~TomboyServerAuthenticateJob() override;

    QString getRequestToken() const;
    QString getRequestTokenSecret() const;
    QString getContentUrl() const;
    QString getUserURL() const;

    void start() override;

private:
    void onLinkingFailed();
    void onLinkingSucceeded();
    void onOpenBrowser(const QUrl &url);

    void onApiRequestFinished();
    void onUserRequestFinished();
    QString mUserURL;

    QWebEngineView *mWebView = nullptr;
};

#endif // TOMBOYSERVERAUTHENTICATEJOB_H
