/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    Unit tests for the fan-out of one change notification into per-item calls.

    Two ways to reach the failure paths without talking to Microsoft: a GraphClient
    without an authentication object fails every request before it reaches the
    network, and FakeGraphServer answers /$batch with a scripted list of per-call
    status codes where a real response is needed.
*/

#include "jobs/graphbatchjob.h"
#include "graphclient/auth/graphoauth.h"
#include "graphclient/graphclient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

/// Answers each /$batch sub-request with the next status code from the list it was
/// given (200 once the list is used up).
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

    /// How many sub-requests arrived — the calls the job really issued.
    [[nodiscard]] int served() const
    {
        return mServed;
    }

    /// Throttle the last entry of every batch that has more than one, the way Graph
    /// answers when a mailbox has too many requests in flight.
    void setThrottleLastOfEachBatch(bool throttle)
    {
        mThrottleLast = throttle;
    }

    /// How many /$batch round trips it took.
    [[nodiscard]] int batches() const
    {
        return mBatches;
    }

protected:
    void incomingConnection(qintptr handle) override
    {
        auto socket = new QTcpSocket(this);
        socket->setSocketDescriptor(handle);
        connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
            mRequest += socket->readAll();
            const int headerEnd = mRequest.indexOf("\r\n\r\n");
            if (headerEnd < 0) {
                return; // headers still incomplete
            }
            const int bodyStart = headerEnd + 4;
            const int lengthPos = mRequest.toLower().indexOf("content-length:");
            const int length = lengthPos < 0 ? 0 : mRequest.mid(lengthPos + 15, mRequest.indexOf("\r\n", lengthPos) - lengthPos - 15).trimmed().toInt();
            if (mRequest.size() < bodyStart + length) {
                return; // body still incomplete
            }
            const QByteArray reply = answer(mRequest.mid(bodyStart, length));
            mRequest.clear();
            socket->write(
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Connection: close\r\n"
                "Content-Length: "
                + QByteArray::number(reply.size()) + "\r\n\r\n" + reply);
            socket->disconnectFromHost();
        });
    }

private:
    [[nodiscard]] QByteArray answer(const QByteArray &batchBody)
    {
        ++mBatches;
        QJsonArray responses;
        const QJsonArray requests = QJsonDocument::fromJson(batchBody).object().value(QLatin1String("requests")).toArray();
        for (qsizetype n = 0; n < requests.size(); ++n) {
            const QJsonValue r = requests.at(n);
            int status = mServed < mStatuses.size() ? mStatuses.at(mServed) : 200;
            if (mThrottleLast && requests.size() > 1 && n == requests.size() - 1) {
                status = 429;
            }
            ++mServed;
            QJsonObject resp;
            resp.insert(QStringLiteral("id"), r.toObject().value(QLatin1String("id")));
            resp.insert(QStringLiteral("status"), status);
            if (status == 429) {
                resp.insert(QStringLiteral("headers"), QJsonObject{{QStringLiteral("Retry-After"), QStringLiteral("1")}});
            }
            resp.insert(QStringLiteral("body"),
                        status < 300 ? QJsonObject{{QStringLiteral("id"), QStringLiteral("moved")}}
                                     : QJsonObject{{QStringLiteral("error"),
                                                    QJsonObject{{QStringLiteral("code"), QStringLiteral("ErrorItemNotFound")},
                                                                {QStringLiteral("message"), QStringLiteral("not found")}}}});
            responses.append(resp);
        }
        return QJsonDocument(QJsonObject{{QStringLiteral("responses"), responses}}).toJson(QJsonDocument::Compact);
    }

    const QList<int> mStatuses;
    QByteArray mRequest;
    int mServed = 0;
    int mBatches = 0;
    bool mThrottleLast = false;
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
        return spy.wait(30000);
    }

    struct LiveClient {
        explicit LiveClient(const FakeGraphServer &server)
            : auth(QStringLiteral("tenant"), QStringLiteral("client"), QStringLiteral("wallet"))
        {
            client.setAuth(&auth);
            client.setBaseUrl(server.baseUrl());
        }
        GraphOAuth auth;
        GraphClient client;
    };

private Q_SLOTS:
    void shouldSucceedWithoutCalls()
    {
        GraphClient client;
        auto job = new GraphBatchJob(client, {}, this);
        QVERIFY(run(job));
        QCOMPARE(job->error(), 0);
        QVERIFY(job->responses().isEmpty());
    }

    void shouldAccountForEveryCallWhenTheServerIsUnreachable()
    {
        // No authentication: the batch request itself fails before the network.
        GraphClient client;
        auto job = new GraphBatchJob(client, deleteCalls(5), this);
        QVERIFY(run(job));
        QVERIFY(job->error() != 0);
        // One response per call, so callers can still match responses to items.
        QCOMPARE(job->responses().size(), 5);
        // Not translated here (no catalog is loaded), so the source string applies.
        QVERIFY(job->errorText().contains(QLatin1String("5 of 5")));
    }

    void shouldReportASingleFailureVerbatim()
    {
        GraphClient client;
        auto job = new GraphBatchJob(client, deleteCalls(1), this);
        QVERIFY(run(job));
        // No "1 of 1" noise around the only error there is.
        QCOMPARE(job->errorText(), QStringLiteral("Not authenticated with Microsoft 365 yet"));
    }

    void shouldBundleTwentyCallsPerRoundTrip()
    {
        FakeGraphServer server({});
        LiveClient live(server);
        auto job = new GraphBatchJob(live.client, deleteCalls(25), this);
        QVERIFY(run(job));
        QCOMPARE(job->error(), 0);
        QCOMPARE(server.batches(), 2);
        QCOMPARE(server.served(), 25);
        QCOMPARE(job->responses().size(), 25);
        for (const QJsonObject &response : job->responses()) {
            QCOMPARE(response.value(QLatin1String("id")).toString(), QStringLiteral("moved"));
        }
    }

    void shouldTolerateAMissingItemWhenAsked()
    {
        // Deleting or moving a message that is already gone server-side must not
        // cost the other messages in the same notification their call.
        FakeGraphServer server({200, 404, 200});
        LiveClient live(server);
        auto job = new GraphBatchJob(live.client, deleteCalls(3), this);
        job->setIgnoreNotFound(true);
        QVERIFY(run(job));
        QCOMPARE(job->error(), 0);
        QCOMPARE(server.served(), 3);
        QCOMPARE(job->responses().size(), 3);
    }

    void shouldIssueEveryCallAfterAServerError()
    {
        FakeGraphServer server({200, 500, 200});
        LiveClient live(server);
        auto job = new GraphBatchJob(live.client, deleteCalls(3), this);
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

    void shouldKeepRetryingWhileThrottledCallsProgress()
    {
        // Six rounds in a row throttle a different call each. Every call is throttled
        // only once, so nothing may count as failed however many rounds it takes.
        FakeGraphServer server({});
        server.setThrottleLastOfEachBatch(true);
        LiveClient live(server);
        auto job = new GraphBatchJob(live.client, deleteCalls(100), this);
        QVERIFY(run(job));
        QCOMPARE(job->error(), 0);
        QCOMPARE(server.batches(), 7);
        QCOMPARE(server.served(), 106);
    }

    void shouldRetryThrottledCalls()
    {
        // Graph throttles per sub-request; the second one is told to come back later.
        FakeGraphServer server({200, 429, 200});
        LiveClient live(server);
        auto job = new GraphBatchJob(live.client, deleteCalls(3), this);
        QVERIFY(run(job));
        QCOMPARE(job->error(), 0);
        QCOMPARE(server.batches(), 2);
        QCOMPARE(server.served(), 4);
        QCOMPARE(job->responses().at(1).value(QLatin1String("id")).toString(), QStringLiteral("moved"));
    }
};

QTEST_GUILESS_MAIN(GraphBatchJobTest)

#include "graphbatchjobtest.moc"
