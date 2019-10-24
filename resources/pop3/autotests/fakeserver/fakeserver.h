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

#include <QTcpSocket>
#include <QTcpServer>
#include <QThread>
#include <QMutex>

class FakeServer : public QObject
{
    Q_OBJECT

public:
    FakeServer(QObject *parent = nullptr);
    ~FakeServer();

    void setNextConversation(const QString &conversation, const QList<int> &exceptions = QList<int>());
    void setAllowedDeletions(const QString &deleteIds);
    void setAllowedRetrieves(const QString &retrieveIds);
    void setMails(const QList<QByteArray> &mails);

    // This is kind of a hack: The POP3 test needs to know when the POP3 client
    // disconnects from the server. Normally, we could just use a QSignalSpy on the
    // disconnected() signal, but that is not thread-safe. Therefore this hack with the
    // state variable mGotDisconnected
    bool gotDisconnected() const;

    // Returns an integer that is incremented each time the POP3 server receives some
    // data
    int progress() const;

Q_SIGNALS:
    void disconnected();

private Q_SLOTS:

    void newConnection();
    void dataAvailable();
    void slotDisconnected();

private:

    QByteArray parseDeleteMark(const QByteArray &expectedData, const QByteArray &dataReceived);
    QByteArray parseRetrMark(const QByteArray &expectedData, const QByteArray &dataReceived);
    QByteArray parseResponse(const QByteArray &expectedData, const QByteArray &dataReceived);

    QList<QByteArray> mReadData;
    QList<QByteArray> mWriteData;
    QList<QByteArray> mAllowedDeletions;
    QList<QByteArray> mAllowedRetrieves;
    QList<QByteArray> mMails;
    QTcpServer *mTcpServer = nullptr;
    QTcpSocket *mTcpServerConnection = nullptr;
    int mConnections;
    int mProgress;
    bool mGotDisconnected = false;

    // We use one big mutex to protect everything
    // There shouldn't be deadlocks, as there are only 2 places where the functions
    // are called: From the KTcpSocket (or QSslSocket with KIO >= 5.65) signals, which
    // are triggered by the POP3 ioslave, and from the actual test.
    mutable QMutex mMutex;
};

class FakeServerThread : public QThread
{
    Q_OBJECT

public:

    explicit FakeServerThread(QObject *parent);
    void run() override;

    // Returns the FakeServer use. Be careful when using this and make sure
    // the methods you use are actually thread-safe!!
    // This should however be the case because the FakeServer uses one big mutex
    // to protect everything.
    FakeServer *server() const;

private:

    FakeServer *mServer = nullptr;
};

#endif
