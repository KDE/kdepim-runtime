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

#include "imapaccount.h"

ImapAccount::ImapAccount()
    : m_encryption(KIMAP::LoginJob::Unencrypted)
    , m_authentication(KIMAP::LoginJob::ClearText)
{
}

ImapAccount::~ImapAccount()
{
}

void ImapAccount::setServer(const QString &server)
{
    m_server = server;
}

QString ImapAccount::server() const
{
    return m_server;
}

void ImapAccount::setPort(quint16 port)
{
    m_port = port;
}

quint16 ImapAccount::port() const
{
    return m_port;
}

void ImapAccount::setUserName(const QString &userName)
{
    m_userName = userName;
}

QString ImapAccount::userName() const
{
    return m_userName;
}

void ImapAccount::setEncryptionMode(KIMAP::LoginJob::EncryptionMode mode)
{
    m_encryption = mode;
}

KIMAP::LoginJob::EncryptionMode ImapAccount::encryptionMode() const
{
    return m_encryption;
}

void ImapAccount::setAuthenticationMode(KIMAP::LoginJob::AuthenticationMode mode)
{
    m_authentication = mode;
}

KIMAP::LoginJob::AuthenticationMode ImapAccount::authenticationMode() const
{
    return m_authentication;
}

void ImapAccount::setSubscriptionEnabled(bool enabled)
{
    m_subscriptionEnabled = enabled;
}

bool ImapAccount::isSubscriptionEnabled() const
{
    return m_subscriptionEnabled;
}

void ImapAccount::setTimeout(int timeout)
{
    m_timeout = timeout;
}

int ImapAccount::timeout() const
{
    return m_timeout;
}
