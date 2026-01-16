/*
   SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
   SPDX-FileContributor: Kevin Ottens <kevin@kdab.com>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "imaptestbase.h"
#include <QSignalSpy>
#include <QTimer>

ImapTestBase::ImapTestBase(QObject *parent)
    : QObject(parent)
{
}

QString ImapTestBase::defaultUserName() const
{
    return QStringLiteral("test@kdab.com");
}

QString ImapTestBase::defaultPassword() const
{
    return QStringLiteral("foobar");
}

ImapAccount *ImapTestBase::createDefaultAccount() const
{
    auto account = new ImapAccount;

    account->setServer(QStringLiteral("127.0.0.1"));
    account->setPort(5989);
    account->setUserName(defaultUserName());
    account->setSubscriptionEnabled(true);
    account->setEncryptionMode(KIMAP::LoginJob::Unencrypted);
    account->setAuthenticationMode(KIMAP::LoginJob::ClearText);

    return account;
}

DummyPasswordRequester *ImapTestBase::createDefaultRequester()
{
    auto requester = new DummyPasswordRequester(this);
    requester->setPassword(defaultPassword());
    return requester;
}

void ImapTestBase::setupTestCase()
{
    qRegisterMetaType<ImapAccount *>();
    qRegisterMetaType<DummyPasswordRequester *>();
    qRegisterMetaType<DummyResourceState::Ptr>();
    qRegisterMetaType<KIMAP::Session *>();
}

QList<QByteArray> ImapTestBase::defaultAuthScenario() const
{
    QList<QByteArray> scenario;

    scenario << FakeServer::greeting() << R"(C: A000001 LOGIN "test@kdab.com" "foobar")"
             << "S: A000001 OK User Logged in";

    return scenario;
}

QList<QByteArray> ImapTestBase::defaultPoolConnectionScenario(const QList<QByteArray> &customCapabilities) const
{
    QList<QByteArray> scenario;

    QByteArray caps = "S: * CAPABILITY IMAP4 IMAP4rev1 UIDPLUS IDLE";
    for (const QByteArray &cap : customCapabilities) {
        caps += " " + cap;
    }

    scenario << defaultAuthScenario() << "C: A000002 CAPABILITY" << caps << "S: A000002 OK Completed";

    return scenario;
}

Akonadi::Collection ImapTestBase::createCollectionChain(const QString &remoteId) const
{
    QChar separator = remoteId.length() > 0 ? remoteId.at(0) : u'/';

    Akonadi::Collection parent(1);
    parent.setRemoteId(QStringLiteral("root-id"));
    parent.setParentCollection(Akonadi::Collection::root());
    Akonadi::Collection::Id id = 2;

    Akonadi::Collection collection = parent;

    const QStringList collections = remoteId.split(separator, Qt::SkipEmptyParts);
    for (const QString &colId : collections) {
        collection = Akonadi::Collection(id);
        collection.setRemoteId(separator + colId);
        collection.setParentCollection(parent);

        parent = collection;
        id++;
    }

    return collection;
}

#include "moc_imaptestbase.cpp"
