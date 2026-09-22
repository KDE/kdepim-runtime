/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    Unit tests for the fan-out of one change notification into per-item calls.

    Two ways to drive the failure paths without talking to Microsoft: a GraphClient
    without an authentication object fails every request before it reaches the
    network, and FakeGraphServer answers a scripted list of HTTP status codes for
    the cases that need a real response.
*/

#include "jobs/graphbatchjob.h"
#include "graphclient/auth/graphoauth.h"
#include "graphclient/graphclient.h"

#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

/// Answers each request with the next status code from the list it was given.
class FakeGraphServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit FakeGraphServer(QList<int> statuses, QObject *parent = nullptr)
        : QTcpServer(parent)
        , mStatuses(std::move(statuses))
    {
        listen(QHostAddress::LocalHost);
    }

    [[nodiscard]] QString baseUrl() const
    {
        return QStringLiteral("http://127.0.0.1:%1").arg(serverPort());
    }

    /// How many requests actually arrived — the calls the job really issued.
    [[nodiscard]] int served() const
    {
        return mServed;
    }

protected:
    void incomingConnection(qintptr handle) override
    {
        auto socket = new QTcpSocket(this);
        socket->setSocketDescriptor(handle);
        connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
            mRequest += socket->readAll();
            if (!mRequest.contains("\r\n\r\n")) {
                return; // headers still incomplete
            }
            mRequest.clear();
            const int status = mServed < mStatuses.size() ? mStatuses.at(mServed) : 200;
            ++mServed;
            const QByteArray body =
                status < 300 ? QByteArrayLiteral(R"({"id":"moved"})") : QByteArrayLiteral(R"({"error":{"code":"ErrorItemNotFound","message":"not found"}})");
            socket->write("HTTP/1.1 " + QByteArray::number(status)
                          + " Status\r\n"
                            "Content-Type: application/json\r\n"
                            "Connection: close\r\n"
                            "Content-Length: "
                          + QByteArray::number(body.size()) + "\r\n\r\n" + body);
            socket->disconnectFromHost();
        });
    }

private:
    const QList<int> mStatuses;
    QByteArray mRequest;
    int mServed = 0;
};

class GraphBatchJobTest : public QObject
{
    Q_OBJECT
private:
    [[nodiscard]] static QList<GraphBatchJob::Call> deleteCalls(int count)
    {
        QList<GraphBatchJob::Call> calls;
        calls.reserve(count);
        for (int i = 0; i < count; ++i) {
            calls.append({GraphRequest::Method::Delete, QStringLiteral("/me/messages/id%1").arg(i), {}});
        }
        return calls;
    }

    [[nodiscard]] static bool run(GraphBatchJob *job)
    {
        QSignalSpy spy(job, &KJob::result);
        job->start();
        return spy.wait();
    }

private Q_SLOTS:
    void shouldSucceedWithoutCalls()
    {
        GraphClient client;
        auto job = new GraphBatchJob(client, {}, this);
        QVERIFY(run(job));
        QCOMPARE(job->error(), 0);
        QVERIFY(job->responses().isEmpty());
    }

    void shouldIssueEveryCallEvenWhenTheyFail()
    {
        // The whole point of the fan-out: one failing item must not cost the others
        // their call, because a cancelled change replay is dropped, not retried.
        GraphClient client;
        auto job = new GraphBatchJob(client, deleteCalls(5), this);
        QVERIFY(run(job));
        QVERIFY(job->error() != 0);
        // One response per call, so callers can still match responses to items.
        QCOMPARE(job->responses().size(), 5);
    }

    void shouldReportHowManyCallsFailed()
    {
        GraphClient client;
        auto job = new GraphBatchJob(client, deleteCalls(3), this);
        QVERIFY(run(job));
        // Not translated here (no catalog is loaded), so the source string applies.
        QVERIFY(job->errorText().contains(QLatin1String("3 of 3")));
    }

    void shouldTolerateAMissingItemWhenAsked()
    {
        // Deleting or moving a message that is already gone server-side must not
        // cost the other messages in the same notification their call.
        FakeGraphServer server({200, 404, 200});
        GraphOAuth auth(QStringLiteral("tenant"), QStringLiteral("client"), QStringLiteral("wallet"));
        GraphClient client;
        client.setAuth(&auth);
        client.setBaseUrl(server.baseUrl());

        auto job = new GraphBatchJob(client, deleteCalls(3), this);
        job->setIgnoreNotFound(true);
        QVERIFY(run(job));
        QCOMPARE(job->error(), 0);
        QCOMPARE(server.served(), 3);
        QCOMPARE(job->responses().size(), 3);
    }

    void shouldIssueEveryCallAfterAServerError()
    {
        FakeGraphServer server({200, 500, 200});
        GraphOAuth auth(QStringLiteral("tenant"), QStringLiteral("client"), QStringLiteral("wallet"));
        GraphClient client;
        client.setAuth(&auth);
        client.setBaseUrl(server.baseUrl());

        auto job = new GraphBatchJob(client, deleteCalls(3), this);
        QVERIFY(run(job));
        // The call after the failing one still went out.
        QCOMPARE(server.served(), 3);
        QCOMPARE(job->responses().size(), 3);
        QVERIFY(job->error() != 0);
        QVERIFY(job->errorText().contains(QLatin1String("1 of 3")));
        // Successful calls keep their response (itemsMoved() reads the new ids from
        // it); the failed one is a placeholder.
        QCOMPARE(job->responses().at(0).value(QLatin1String("id")).toString(), QStringLiteral("moved"));
        QVERIFY(job->responses().at(1).isEmpty());
        QCOMPARE(job->responses().at(2).value(QLatin1String("id")).toString(), QStringLiteral("moved"));
    }

    void shouldReportASingleFailureVerbatim()
    {
        GraphClient client;
        auto job = new GraphBatchJob(client, deleteCalls(1), this);
        QVERIFY(run(job));
        // No "1 of 1" noise around the only error there is.
        QCOMPARE(job->errorText(), QStringLiteral("Not authenticated with Microsoft 365 yet"));
    }
};

QTEST_GUILESS_MAIN(GraphBatchJobTest)

#include "graphbatchjobtest.moc"
