/*
    SPDX-FileCopyrightText: 2026 Stefan Huber <shuber@sthu.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <memory>

#include <QPointer>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

#include "ewsclient.h"
#include "ewsgetstreamingeventsrequest.h"

using namespace Qt::Literals::StringLiterals;

namespace
{

/* A GetStreamingEvents response: the server sends one SOAP document per
 * notification batch, and it keeps the connection open in between. */
QByteArray streamingResponse(const QByteArray &notifications)
{
    return R"(<?xml version="1.0" encoding="utf-8"?>)"
           R"(<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" )"
           R"(xmlns:m="http://schemas.microsoft.com/exchange/services/2006/messages" )"
           R"(xmlns:t="http://schemas.microsoft.com/exchange/services/2006/types">)"
           R"(<soap:Header><t:ServerVersionInfo MajorVersion="15" MinorVersion="20" MajorBuildNumber="1" MinorBuildNumber="0" /></soap:Header>)"
           R"(<soap:Body><m:GetStreamingEventsResponse><m:ResponseMessages>)"
           R"(<m:GetStreamingEventsResponseMessage ResponseClass="Success">)"
           R"(<m:ResponseCode>NoError</m:ResponseCode>)"
        + notifications
        + R"(<m:ConnectionStatus>OK</m:ConnectionStatus>)"
          R"(</m:GetStreamingEventsResponseMessage>)"
          R"(</m:ResponseMessages></m:GetStreamingEventsResponse></soap:Body></soap:Envelope>)";
}

const QByteArray newMailNotification = R"(<m:Notifications><m:Notification>)"
                                       R"(<t:SubscriptionId>sub1</t:SubscriptionId>)"
                                       R"(<t:CreatedEvent>)"
                                       R"(<t:TimeStamp>2026-11-19T10:00:00Z</t:TimeStamp>)"
                                       R"(<t:ItemId Id="item1" ChangeKey="ck1" />)"
                                       R"(<t:ParentFolderId Id="calendar" ChangeKey="ck2" />)"
                                       R"(</t:CreatedEvent>)"
                                       R"(</m:Notification></m:Notifications>)";

QByteArray chunk(const QByteArray &data)
{
    return QByteArray::number(data.size(), 16) + "\r\n" + data + "\r\n";
}

/* A local server that accepts the request and lets the test write the response. */
class StreamingServer
{
public:
    StreamingServer()
    {
        QObject::connect(&mServer, &QTcpServer::newConnection, &mServer, [this]() {
            conn = mServer.nextPendingConnection();
            QObject::connect(conn, &QTcpSocket::readyRead, conn, [this]() {
                requestData += conn->readAll();
            });
        });
    }

    bool listen()
    {
        return mServer.listen(QHostAddress::LocalHost);
    }

    QString url() const
    {
        return u"http://127.0.0.1:%1/EWS/Exchange.asmx"_s.arg(mServer.serverPort());
    }

    void writeResponseHeader()
    {
        conn->write(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/xml; charset=utf-8\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n");
    }

    QPointer<QTcpSocket> conn;
    QByteArray requestData;

private:
    QTcpServer mServer;
};

}

class UtEwsGetStreamingEventsRequest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void eventsWhileConnectionOpen();
    void endWithoutFinalDocument();
};

/* Notifications have to be delivered while the server keeps the connection
 * open, not only once the response is complete, which happens after the
 * connection timeout of up to 30 minutes. */
void UtEwsGetStreamingEventsRequest::eventsWhileConnectionOpen()
{
    StreamingServer server;
    QVERIFY(server.listen());

    EwsClient client;
    client.setUrl(server.url());

    // A finished job deletes itself, but the checks below still use the request.
    std::unique_ptr<EwsGetStreamingEventsRequest> req(new EwsGetStreamingEventsRequest(client, nullptr));
    req->setAutoDelete(false);
    req->setSubscriptionId(u"sub1"_s);
    req->setTimeout(std::chrono::seconds(30 * 60));
    QSignalSpy eventsSpy(req.get(), &EwsGetStreamingEventsRequest::eventsReceived);
    QSignalSpy resultSpy(req.get(), &KJob::result);
    req->start();

    QTRY_VERIFY(server.requestData.contains("</soap:Envelope>"));
    QVERIFY(server.requestData.contains("GetStreamingEvents"));

    server.writeResponseHeader();
    server.conn->write(chunk(streamingResponse(newMailNotification)));

    QTRY_COMPARE(eventsSpy.count(), 1);
    QCOMPARE(resultSpy.count(), 0);

    QCOMPARE(req->responses().size(), 1);
    const auto &resp = req->responses().first();
    QVERIFY(resp.isSuccess());
    QCOMPARE(resp.notifications().size(), 1);
    const auto &events = resp.notifications().first().events();
    QCOMPARE(events.size(), 1);
    QCOMPARE(events.first().type(), EwsCreatedEvent);
    QCOMPARE(events.first().parentFolderId().id(), u"calendar"_s);
    req->eventsProcessed(resp);

    // A heartbeat without notifications.
    server.conn->write(chunk(streamingResponse({})));
    QTRY_COMPARE(eventsSpy.count(), 2);
    QCOMPARE(resultSpy.count(), 0);
    QCOMPARE(req->responses().size(), 1);
    QVERIFY(req->responses().first().notifications().isEmpty());
    req->eventsProcessed(req->responses().first());

    // The server ends the response on the connection timeout.
    server.conn->write(chunk(streamingResponse({})) + "0\r\n\r\n");
    QTRY_COMPARE(resultSpy.count(), 1);
    QCOMPARE(req->error(), 0);
}

/* The server may end the response without a final document after the last one
 * has been parsed already. This is a normal end, not an error. */
void UtEwsGetStreamingEventsRequest::endWithoutFinalDocument()
{
    StreamingServer server;
    QVERIFY(server.listen());

    EwsClient client;
    client.setUrl(server.url());

    // A finished job deletes itself, but the checks below still use the request.
    std::unique_ptr<EwsGetStreamingEventsRequest> req(new EwsGetStreamingEventsRequest(client, nullptr));
    req->setAutoDelete(false);
    req->setSubscriptionId(u"sub1"_s);
    QSignalSpy eventsSpy(req.get(), &EwsGetStreamingEventsRequest::eventsReceived);
    QSignalSpy resultSpy(req.get(), &KJob::result);
    req->start();

    QTRY_VERIFY(server.requestData.contains("</soap:Envelope>"));

    server.writeResponseHeader();
    server.conn->write(chunk(streamingResponse({})));
    QTRY_COMPARE(eventsSpy.count(), 1);
    req->eventsProcessed(req->responses().first());

    server.conn->write("0\r\n\r\n");
    QTRY_COMPARE(resultSpy.count(), 1);
    QCOMPARE(req->error(), 0);
    QCOMPARE(eventsSpy.count(), 1);
}

QTEST_GUILESS_MAIN(UtEwsGetStreamingEventsRequest)

#include "ewsgetstreamingeventsrequest_ut.moc"
