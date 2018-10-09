/*
    Copyright (C) 2018 Krzysztof Nowicki <krissn@op.pl>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef EWSOAUTH_H
#define EWSOAUTH_H

#include <QObject>
#include <QScopedPointer>

class QWidget;
class EwsOAuthPrivate;

class EwsOAuth : public QObject
{
    Q_OBJECT
public:
    enum State {
        NotAuthenticated,
        Authenticating,
        Authenticated,
        AuthenticationFailed
    };

    EwsOAuth(QObject *parent, const QString &email, const QString &appId, const QString &redirectUri);
    ~EwsOAuth() override;

    void authenticate();
    QString token() const;
    State state() const;
    QString refreshToken() const;

    void setParentWindow(QWidget *w);

    void setAccessToken(const QString &accessToken);
    void setRefreshToken(const QString &refreshToken);
    void resetAccessToken();
    void browserDisplayReply(bool display);
Q_SIGNALS:
    void browserDisplayRequest();
    void granted();
    void error(const QString &error, const QString &errorDescription, const QUrl &uri);
private:
    QScopedPointer<EwsOAuthPrivate> d_ptr;
    Q_DECLARE_PRIVATE(EwsOAuth)
};

#endif /* EWSOAUTH_H */
