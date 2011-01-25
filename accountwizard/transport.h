/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "setupobject.h"
#include <mailtransport/transport.h>


class Transport : public SetupObject
{
  Q_OBJECT
  public:
    explicit Transport( const QString &type, QObject *parent = 0 );
    void create();
    void destroy();

    int transportId() const;

  public slots:
    Q_SCRIPTABLE void setName( const QString &name );
    Q_SCRIPTABLE void setHost( const QString &host );
    Q_SCRIPTABLE void setPort( int port );
    Q_SCRIPTABLE void setUsername( const QString &user );
    Q_SCRIPTABLE void setPassword( const QString &password );
    Q_SCRIPTABLE void setEncryption( const QString &encryption );
    Q_SCRIPTABLE void setAuthenticationType( const QString &authType );

  private:
    MailTransport::Transport::EnumEncryption stringToEncryption( const QString &encryptionString );
    MailTransport::Transport::EnumAuthenticationType stringToAuthType( const QString &authString );

  private:
    int m_transportId;
    MailTransport::Transport::EnumType::type m_transportType;
    QString m_name;
    QString m_host;
    int m_port;
    QString m_user;
    QString m_password;
    MailTransport::Transport::EnumEncryption::type m_encr;
    MailTransport::Transport::EnumAuthenticationType::type m_auth;
};

#endif
