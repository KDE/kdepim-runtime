/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
    SPDX-FileContributor: Kevin Krammer <krake@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

template<typename T> class QList;

namespace Akonadi
{
class Collection;
class Item;

typedef QVector<Item> ItemList;
}

class CompactChangeHelper : public QObject
{
    Q_OBJECT

public:
    explicit CompactChangeHelper(const QByteArray &sessionId, QObject *parent = nullptr);

    ~CompactChangeHelper();

    void addChangedItems(const Akonadi::ItemList &items);

    QString currentRemoteId(const Akonadi::Item &item) const;

    void checkCollectionChanged(const Akonadi::Collection &collection);

private:
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void processNextBatch())
    Q_PRIVATE_SLOT(d, void processNextItem())
};

