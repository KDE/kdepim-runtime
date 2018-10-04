/*
    Copyright (C) 2017 Krzysztof Nowicki <krissn@op.pl>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <AkonadiCore/AgentInstanceCreateJob>
#include <AkonadiCore/AgentManager>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/Control>
#include <AkonadiCore/SpecialCollectionAttribute>
#include <AkonadiCore/Monitor>
#include <qtest_akonadi.h>

#include "fakeewsserverthread.h"
#include "ewssettings.h"
#include "ewswallet.h"
#include "isolatedtestbase.h"
#include "statemonitor.h"

class BasicTest : public IsolatedTestBase
{
    Q_OBJECT
public:
    explicit BasicTest(QObject *parent = nullptr);
    ~BasicTest() override;
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testBasic();
};

QTEST_AKONADIMAIN(BasicTest)

using namespace Akonadi;

constexpr int DesiredStateTimeoutMs = 20000;

BasicTest::BasicTest(QObject *parent)
    : IsolatedTestBase(parent)
{
}

BasicTest::~BasicTest()
{
}

void BasicTest::initTestCase()
{
    init();
}

void BasicTest::cleanupTestCase()
{
    cleanup();
}

void BasicTest::testBasic()
{
    TestAgentInstance instance(QStringLiteral("http://127.0.0.1:%1/EWS/Exchange.asmx").arg(mFakeServerThread->portNumber()));
    QVERIFY(instance.isValid());

    static const auto rootId = QStringLiteral("cm9vdA==");
    static const auto inboxId = QStringLiteral("aW5ib3g=");
    FolderList folderList = {
        {rootId, instance.identifier(), Folder::Root, QString()},
        {inboxId, QStringLiteral("Inbox"), Folder::Inbox, rootId},
        {QStringLiteral("Y2FsZW5kYXI="), QStringLiteral("Calendar"), Folder::Calendar, rootId},
        {QStringLiteral("dGFza3M="), QStringLiteral("Tasks"), Folder::Tasks, rootId},
        {QStringLiteral("Y29udGFjdHM="), QStringLiteral("Contacts"), Folder::Contacts, rootId},
        {QStringLiteral("b3V0Ym94"), QStringLiteral("Outbox"), Folder::Outbox, rootId},
        {QStringLiteral("c2VudCBpdGVtcw=="), QStringLiteral("Sent Items"), Folder::Sent, rootId},
        {QStringLiteral("ZGVsZXRlZCBpdGVtcw=="), QStringLiteral("Deleted Items"), Folder::Trash, rootId},
        {QStringLiteral("ZHJhZnRz"), QStringLiteral("Drafts"), Folder::Drafts, rootId}
    };

    struct DesiredState {
        QString parentId;
        QByteArray specialType;
    };
    QHash<QString, DesiredState> desiredStates = {
        {rootId, {QString(), QByteArray()}},
        {inboxId, {rootId, "inbox"}},
        {QStringLiteral("Y2FsZW5kYXI="), {rootId, QByteArray()}},
        {QStringLiteral("dGFza3M="), {rootId, QByteArray()}},
        {QStringLiteral("Y29udGFjdHM="), {rootId, QByteArray()}},
        {QStringLiteral("b3V0Ym94"), {rootId, "outbox"}},
        {QStringLiteral("c2VudCBpdGVtcw=="), {rootId, "sent-mail"}},
        {QStringLiteral("ZGVsZXRlZCBpdGVtcw=="), {rootId, "trash"}},
        {QStringLiteral("ZHJhZnRz"), {rootId, "drafts"}}
    };

    FakeEwsServer::DialogEntry::List dialog = {
        MsgRootInboxDialogEntry(rootId, inboxId,
                                QStringLiteral("GetFolder request for inbox and msgroot")),
        SubscribedFoldersDialogEntry(folderList,
                                     QStringLiteral("GetFolder request for subscribed folders")),
        ValidateFolderIdsDialogEntry(QStringList()
                                     << inboxId
                                     << QStringLiteral("Y2FsZW5kYXI=")
                                     << QStringLiteral("dGFza3M=")
                                     << QStringLiteral("Y29udGFjdHM="),
                                     QStringLiteral("GetFolder request for subscribed folder ids")),
        SpecialFoldersDialogEntry(folderList,
                                  QStringLiteral("GetFolder request for special folders")),
        GetTagsEmptyDialogEntry(rootId,
                                QStringLiteral("GetFolder request for tags")),
        SubscribeStreamingDialogEntry(QStringLiteral("Subscribe request for streaming events")),
        SyncFolderHierInitialDialogEntry(folderList, QStringLiteral("bNUUPDWHTvuG9p57NGZdhjREdZXDt48a0E1F22yThko="),
                                         QStringLiteral("SyncFolderHierarchy request with empty state")),
        UnsubscribeDialogEntry(QStringLiteral("Unsubscribe request"))
    };

    bool unknownRequestEncountered = false;
    mFakeServerThread->setDialog(dialog);
    mFakeServerThread->setDefaultReplyCallback([&](const QString & req, QXmlResultItems &, const QXmlNamePool &) {
        qDebug() << "Unknown EWS request encountered." << req;
        unknownRequestEncountered = true;
        return FakeEwsServer::EmptyResponse;
    });

    QEventLoop loop;

    CollectionStateMonitor<DesiredState> stateMonitor(this, desiredStates, inboxId,
                                                      [](const Collection &col, const DesiredState &state) {
        auto attr = col.attribute<SpecialCollectionAttribute>();
        QByteArray specialType;
        if (attr) {
            specialType = attr->collectionType();
        }
        return col.parentCollection().remoteId() == state.parentId && specialType == state.specialType;
    });

    stateMonitor.monitor().fetchCollection(true);
    stateMonitor.monitor().collectionFetchScope().fetchAttribute<SpecialCollectionAttribute>();
    stateMonitor.monitor().collectionFetchScope().setAncestorRetrieval(CollectionFetchScope::Parent);
    stateMonitor.monitor().setResourceMonitored(instance.identifier().toLatin1(), true);
    connect(&stateMonitor, &CollectionStateMonitor<DesiredState>::stateReached, this, [&]() {
        loop.exit(0);
    });
    connect(&stateMonitor, &CollectionStateMonitor<DesiredState>::errorOccurred, this, [&]() {
        loop.exit(1);
    });

    QVERIFY(instance.setOnline(true, true));

    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, [&]() {
        qWarning() << "Timeout waiting for desired resource online state.";
        loop.exit(1);
    });
    timer.start(DesiredStateTimeoutMs);
    QCOMPARE(loop.exec(), 0);

    QVERIFY(!unknownRequestEncountered);
    QVERIFY(instance.setOnline(false, true));
}

#include "ewstest.moc"
