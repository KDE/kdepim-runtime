/*
    SPDX-FileCopyrightText: 2015-2019 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <functional>

#include "fakeewsserver_export.h"

#include <QMutex>
#include <QPointer>
#include <QTcpServer>

class FakeEwsConnection;
class QDomElement;

class FAKEEWSSERVER_EXPORT FakeEwsServer : public QTcpServer
{
    Q_OBJECT
public:
    class FAKEEWSSERVER_EXPORT DialogEntry
    {
    public:
        using HttpResponse = std::pair<QString, ushort>;
        /* Decides whether the dialog entry answers a request. The request is handed over as
         * the root element of its XML document. */
        using Matcher = std::function<bool(const QDomElement &request)>;
        // Builds the response for a request the entry matched.
        using ReplyCallback = std::function<HttpResponse(const QString &request)>;

        DialogEntry() = default;
        // An entry that answers a matched request with a fixed response.
        DialogEntry(Matcher matcher, const QString &response, const QString &description = QString());
        // An entry that lets a callback build the response.
        DialogEntry(Matcher matcher, ReplyCallback replyCallback, const QString &description = QString());

        Matcher matcher;
        QString response;
        ReplyCallback replyCallback;
        QString description;

        using List = QList<DialogEntry>;
    };

    static const DialogEntry::HttpResponse EmptyResponse;

    explicit FakeEwsServer(QObject *parent);
    ~FakeEwsServer() override;
    bool start();
    void setDefaultReplyCallback(const DialogEntry::ReplyCallback &defaultReplyCallback);
    void setOverrideReplyCallback(const DialogEntry::ReplyCallback &overrideReplyCallback);
    void queueEventsXml(const QStringList &events);
    void setDialog(const DialogEntry::List &dialog);
    ushort portNumber() const;
private Q_SLOTS:
    void newConnectionReceived();
    void streamingConnectionStarted(FakeEwsConnection *conn);

private:
    const DialogEntry::List dialog() const;
    const DialogEntry::ReplyCallback defaultReplyCallback() const;
    const DialogEntry::ReplyCallback overrideReplyCallback() const;
    QStringList retrieveEventsXml();

    DialogEntry::List mDialog;
    DialogEntry::ReplyCallback mDefaultReplyCallback;
    DialogEntry::ReplyCallback mOverrideReplyCallback;
    QStringList mEventQueue;
    QPointer<FakeEwsConnection> mStreamingEventsConnection;
    ushort mPortNumber;
    mutable QMutex mMutex;

    friend class FakeEwsConnection;
};
