/*
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>
    Copyright (C) 2009 Kevin Ottens <ervin@kde.org>

    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#ifndef IMAPACCOUNT_H
#define IMAPACCOUNT_H

#include <kimap/loginjob.h>

class ImapAccount
{
public:
    ImapAccount();
    ~ImapAccount();

    void setServer(const QString &server);
    QString server() const;

    void setPort(quint16 port);
    quint16 port() const;

    void setUserName(const QString &userName);
    QString userName() const;

    void setEncryptionMode(KIMAP::LoginJob::EncryptionMode mode);
    KIMAP::LoginJob::EncryptionMode encryptionMode() const;

    void setAuthenticationMode(KIMAP::LoginJob::AuthenticationMode mode);
    KIMAP::LoginJob::AuthenticationMode authenticationMode() const;

    void setSubscriptionEnabled(bool enabled);
    bool isSubscriptionEnabled() const;

    void setTimeout(int timeout);
    int timeout() const;

private:
    QString m_name;
    QString m_server;
    quint16 m_port = 0;
    QString m_userName;
    int m_timeout = 30;
    KIMAP::LoginJob::EncryptionMode m_encryption;
    KIMAP::LoginJob::AuthenticationMode m_authentication;
    bool m_subscriptionEnabled = false;
};

#endif
