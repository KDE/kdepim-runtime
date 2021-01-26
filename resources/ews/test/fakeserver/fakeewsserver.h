/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef FAKEEWSSERVER_H
#define FAKEEWSSERVER_H

#include <functional>

#include "fakeewsserver_export.h"

#include <QMutex>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>

class FakeEwsConnection;
class QXmlResultItems;
class QXmlNamePool;

class FAKEEWSSERVER_EXPORT FakeEwsServer : public QTcpServer
{
    Q_OBJECT
public:
    class FAKEEWSSERVER_EXPORT DialogEntry
    {
    public:
        typedef QPair<QString, ushort> HttpResponse;
        typedef std::function<HttpResponse(const QString &, QXmlResultItems &, const QXmlNamePool &)> ReplyCallback;
        QString xQuery;
        ReplyCallback replyCallback;
        QString description;

        typedef QVector<DialogEntry> List;
    };

    static const DialogEntry::HttpResponse EmptyResponse;

    explicit FakeEwsServer(QObject *parent);
    ~FakeEwsServer() override;
    bool start();
    void setDefaultReplyCallback(const DialogEntry::ReplyCallback &defaultReplyCallback);
    void queueEventsXml(const QStringList &events);
    void setDialog(const DialogEntry::List &dialog);
    ushort portNumber() const;
private Q_SLOTS:
    void newConnectionReceived();
    void streamingConnectionStarted(FakeEwsConnection *conn);

private:
    void dataAvailable(QTcpSocket *sock);
    void sendError(QTcpSocket *sock, const QString &msg, ushort code = 500);
    const DialogEntry::List dialog() const;
    const DialogEntry::ReplyCallback defaultReplyCallback() const;
    QStringList retrieveEventsXml();

    DialogEntry::List mDialog;
    DialogEntry::ReplyCallback mDefaultReplyCallback;
    QStringList mEventQueue;
    QPointer<FakeEwsConnection> mStreamingEventsConnection;
    ushort mPortNumber;
    mutable QMutex mMutex;

    friend class FakeEwsConnection;
};

#endif
