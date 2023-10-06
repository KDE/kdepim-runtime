/*
    SPDX-FileCopyrightText: 2007 Till Adam <adam@kde.org>
    SPDX-FileCopyrightText: 2008 Omat Holding B.V. <info@omat.nl>
    SPDX-FileCopyrightText: 2009 Kevin Ottens <ervin@kde.org>

    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    SPDX-FileContributor: Kevin Ottens <kevin@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KIMAP/LoginJob>

class ImapAccount
{
public:
    ImapAccount();
    ~ImapAccount();

    void setServer(const QString &server);
    [[nodiscard]] QString server() const;

    void setPort(quint16 port);
    [[nodiscard]] quint16 port() const;

    void setUserName(const QString &userName);
    [[nodiscard]] QString userName() const;

    void setEncryptionMode(KIMAP::LoginJob::EncryptionMode mode);
    [[nodiscard]] KIMAP::LoginJob::EncryptionMode encryptionMode() const;

    void setAuthenticationMode(KIMAP::LoginJob::AuthenticationMode mode);
    [[nodiscard]] KIMAP::LoginJob::AuthenticationMode authenticationMode() const;

    void setSubscriptionEnabled(bool enabled);
    [[nodiscard]] bool isSubscriptionEnabled() const;

    void setTimeout(int timeout);
    [[nodiscard]] int timeout() const;

    void setUseNetworkProxy(bool useProxy);
    bool useNetworkProxy() const;

private:
    QString m_name;
    QString m_server;
    quint16 m_port = 0;
    QString m_userName;
    int m_timeout = 30;
    bool m_useProxy = false;
    KIMAP::LoginJob::EncryptionMode m_encryption;
    KIMAP::LoginJob::AuthenticationMode m_authentication;
    bool m_subscriptionEnabled = false;
};
