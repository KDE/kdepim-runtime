/*  This file is part of the KDE project
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

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

#ifndef MIXEDMAILDIR_RESOURCE_H
#define MIXEDMAILDIR_RESOURCE_H

#include <resourcebase.h>

#include <QStringList>

class CompactChangeHelper;
class MixedMaildirStore;

class MixedMaildirResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::ObserverV2
{
    Q_OBJECT

public:
    explicit MixedMaildirResource(const QString &id);
    ~MixedMaildirResource();

protected Q_SLOTS:
    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &col) override;
    bool retrieveItems(const Akonadi::Item::List &items, const QSet<QByteArray> &parts) override;

protected:
    void aboutToQuit() override;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
    void itemMoved(const Akonadi::Item &item, const Akonadi::Collection &source, const Akonadi::Collection &dest) override;
    void itemRemoved(const Akonadi::Item &item) override;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override;
    void collectionChanged(const Akonadi::Collection &collection) override;
    void collectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &changedAttributes) override;
    void collectionMoved(const Akonadi::Collection &collection, const Akonadi::Collection &source, const Akonadi::Collection &dest) override;
    void collectionRemoved(const Akonadi::Collection &collection) override;

private:
    bool ensureDirExists();
    bool ensureSaneConfiguration();

    void checkForInvalidatedIndexCollections(KJob *job);

private Q_SLOTS:
    void reapplyConfiguration();

    void retrieveCollectionsResult(KJob *job);
    void retrieveItemsResult(KJob *job);
    void retrieveItemResult(KJob *job);

    void itemAddedResult(KJob *job);
    void itemChangedResult(KJob *job);
    void itemMovedResult(KJob *job);
    void itemRemovedResult(KJob *job);

    void itemsDeleted(KJob *job);

    void collectionAddedResult(KJob *job);
    void collectionChangedResult(KJob *job);
    void collectionMovedResult(KJob *job);
    void collectionRemovedResult(KJob *job);

    void compactStore(const QVariant &arg);
    void compactStoreResult(KJob *job);

    void restoreTags(const QVariant &arg);
    void processNextTagContext();
    void tagFetchJobResult(KJob *job);

private:
    MixedMaildirStore *mStore = nullptr;

    struct TagContext {
        Akonadi::Item mItem;
        QStringList mTagList;
    };

    typedef QList<TagContext> TagContextList;
    QHash<Akonadi::Collection::Id, TagContextList> mTagContextByColId;
    TagContextList mPendingTagContexts;

    QSet<Akonadi::Collection::Id> mSynchronizedCollections;
    QSet<Akonadi::Collection::Id> mPendingSynchronizeCollections;

    CompactChangeHelper *mCompactHelper = nullptr;
};

#endif
