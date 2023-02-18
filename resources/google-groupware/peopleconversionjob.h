/*
   SPDX-FileCopyrightText: 2023 Claudio Cambra <claudio.cambra@kde.org>
   SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <QString>

#include <KGAPI/Types>
#include <Akonadi/Item>

#pragma once

// Job to convert Akonadi items into KGAPI2 People with all the needed data.
class PeopleConversionJob : public QObject
{
    Q_OBJECT

    Q_PROPERTY(KGAPI2::People::PersonList people READ people NOTIFY peopleChanged)

public:
    explicit PeopleConversionJob(const Akonadi::Item::List peopleItems, QObject* parent = nullptr);
    KGAPI2::People::PersonList people() const;

Q_SIGNALS:
    void finished();
    void peopleChanged();

public Q_SLOTS:
    void start();

private Q_SLOTS:
    void jobFinished(KJob *job);
    void processItems();

private:
    Akonadi::Item::List _items;
    QHash<Akonadi::Collection::Id, QString> _localToRemoteIdHash;
    KGAPI2::People::PersonList _processedPeople;
};
