/* This file is part of the KDE project

   Copyright (C) 2006-2007 KovoKs <info@kovoks.nl>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/


#ifndef SOCKETSAFE_H
#define SOCKETSAFE_H

#include <QSslSocket>

/**
 * @class Socket
 * Responsible for communicating with the server with safe IMAPs
 * @author Tom Albers <tomalbers@kde.nl>
 */
class Socket : public QObject
{
    Q_OBJECT

    public:

        /**
         * Contructor, it will not auto connect. Call reconnect() to connect to
         * the parameters given.
         * @param parent the parent
         * @param name the name, will be printed  on debug.
         * @param server the server to connect to
         * @param port the port to connect to
         * @param safe the safety level
         */
        explicit Socket(QObject* parent);

        /**
        * Destructor
        */
        ~Socket();

        /**
         * Call this when you will terminate the connection, it will
         * prevent a popup to the user 'do you want to reconnect'
         */
        void aboutToClose() { m_aboutToClose = true; };

        /**
         * Existing connection will be closed and a new connection will be
         * made
         */
        virtual void reconnect();

        /**
         * Write @p text to the socket
         */
        virtual void write(const QString& text);

        /**
         * @return true when the connection is live and kicking
         */
        virtual bool available();

        /**
         * call this when there are ssl errors and you agree to those
         * and want to connect anyhow.
         */
        void acceptSslErrors();

        /**
         * call this when there are ssl errors and you agree to those
         * and want to connect anyhow.
         */
        void setInteractive( bool ia ) { m_interactive = ia; };

        /**
         * set the protocol to use
         */
        void setProtocol( const QString& proto ) { m_proto = proto; };

        /**
         * set the server to use
         */
        void setServer( const QString& server ) { m_server = server; };

        /**
         * set the port to use. If not specified, it will use the default
         * belonging to the protocol.
         */
        void setPort( int port ) { m_port = port; };

        /**
         * this will be an tls connection
         */
        void setTls( bool what ) { m_tls = what; };

        /**
         * this will be a secure connection
         */
        void setSecure( bool what ) { m_secure = what; };

    private:
        QSslSocket*         m_socket;
        QString             m_server;
        QString             m_proto;
        int                 m_port;

        bool                m_aboutToClose;
        bool                m_interactive;
        bool                m_secure;
        bool                m_tls;

        void login();
        void startShake();


    signals:
        /**
         * emits the incoming data
         */
        void data(const QString&);

        /**
         * emitted when there is a connection (ready to send something)
         */
        void connected();

        /**
         * emitted when the dns request is ok (pretty useless, but still).
         */
        void hostFound();

        /**
         * emitted when there is something wrong, please issue a self
         * dustruct on me or accept the error by calling accept Errors();
         */
        void sslError(const QString&);

        /**
         * emitted when ssl errors are not accepted.
         * Please delete this class when this signal is emitted.
         */
        void disconnected();

        /**
         * emitted when disconnected, but only when the aboutToClose() is
         * not called before. Please delete this class when this signal
         * is emitted.
         */
        void unexpectedDisconnect();

        /**
         * this is the signal when tls succeeded
         */
        void tlscomplete();


    private slots:
        void slotConnected();
        void slotStateChanged( QAbstractSocket::SocketState state);
        void slotModeChanged(  QSslSocket::SslMode  state);
        void slotSocketRead();
        void slotSslErrors(const QList<QSslError> & errors);
        void slotDisconnected();

};

#endif

