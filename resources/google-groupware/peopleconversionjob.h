/*
   SPDX-FileCopyrightText: 2023 Claudio Cambra <claudio.cambra@kde.org>
   SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <QString>

#include <KGAPI/Types>
#include <Akonadi/Item>

#pragma once

namespace KGAPI2::People {
class Membership;
}

// Job to convert Akonadi items into KGAPI2 People with all the needed data.
class PeopleConversionJob : public QObject
{
    Q_OBJECT

    Q_PROPERTY(KGAPI2::People::PersonList people READ people NOTIFY peopleChanged)
    Q_PROPERTY(QString reparentCollectionRemoteId READ reparentCollectionRemoteId WRITE setReparentCollectionRemoteId NOTIFY reparentCollectionRemoteIdChanged)
    Q_PROPERTY(QString newLinkedCollectionRemoteId READ newLinkedCollectionRemoteId WRITE setNewLinkedCollectionRemoteId NOTIFY newLinkedCollectionRemoteIdChanged)

public:
    explicit PeopleConversionJob(const Akonadi::Item::List peopleItems, QObject* parent = nullptr);

    KGAPI2::People::PersonList people() const;
    QString reparentCollectionRemoteId() const;
    QString newLinkedCollectionRemoteId() const;

Q_SIGNALS:
    void finished();
    void peopleChanged();
    void reparentCollectionRemoteIdChanged();
    void newLinkedCollectionRemoteIdChanged();

public Q_SLOTS:
    void start();
    void setReparentCollectionRemoteId(const QString &reparentCollectionRemoteId);
    void setNewLinkedCollectionRemoteId(const QString &newLinkedCollectionRemoteId);

private Q_SLOTS:
    void jobFinished(KJob *job);
    void processItems();

private:
    KGAPI2::People::Membership resourceNameToMembership(const QString &resourceName);

    Akonadi::Item::List _items;
    QHash<Akonadi::Collection::Id, QString> _localToRemoteIdHash;
    KGAPI2::People::PersonList _processedPeople;
    QString _reparentCollectionRemoteId;
    QString _newLinkedCollectionRemoteId;
};