/*
   Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>
   Copyright (C) 2009 Thomas McGuire <mcguire@kde.org>

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


#ifndef FAKESERVER_H
#define FAKESERVER_H

#include <QStringList>
#include <QTcpSocket>
#include <QTcpServer>
#include <QThread>
#include <QMutex>

class FakeServer : public QThread
{
    Q_OBJECT

public:
    FakeServer( QObject* parent = 0 );
    ~FakeServer();
    virtual void run();
    void setNextConversation( const QString &conversation,
                              const QList<int> &exceptions = QList<int>() );
    void setAllowedDeletions( const QString &deleteIds );
    void setMails( const QList<QByteArray> &mails );

Q_SIGNALS:
    void disconnected();
    void progress();
private Q_SLOTS:
    void newConnection();
    void dataAvailable();
    void slotDisconnected();

private:

    QByteArray parseResponse( const QByteArray& expectedData, const QByteArray& dataReceived );

    QList<QByteArray> mReadData;
    QList<QByteArray> mWriteData;
    QList<QByteArray> mAllowedDeletions;
    QList<QByteArray> mMails;
    int mConnections;
    QTcpServer *mTcpServer;
    QMutex mMutex;
    QTcpSocket* mTcpServerConnection;
};

#endif

