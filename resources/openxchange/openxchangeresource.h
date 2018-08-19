/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef OPENXCHANGERESOURCE_H
#define OPENXCHANGERESOURCE_H

#include <resourcebase.h>

class OpenXchangeResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::ObserverV2
{
    Q_OBJECT

public:
    explicit OpenXchangeResource(const QString &id);
    ~OpenXchangeResource();

    void cleanup() override;

public Q_SLOTS:
    void aboutToQuit() override;

protected:
    using ResourceBase::retrieveItems; // Suppress -Woverload-virtual

protected Q_SLOTS:
    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &collection) override;
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;

protected:
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
    void itemRemoved(const Akonadi::Item &item) override;
    virtual void itemMoved(const Akonadi::Item &item, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination) override;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override;
    void collectionChanged(const Akonadi::Collection &collection) override;
    // do not hide the other variant, use implementation from base class
    // which just forwards to the one above
    using Akonadi::AgentBase::ObserverV2::collectionChanged;
    void collectionRemoved(const Akonadi::Collection &collection) override;
    virtual void collectionMoved(const Akonadi::Collection &collection, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination) override;

private Q_SLOTS:
    void onReloadConfiguration();

    void onUpdateUsersJobFinished(KJob *);
    void onFoldersRequestJobFinished(KJob *);
    void onFoldersRequestDeltaJobFinished(KJob *);
    void onFolderCreateJobFinished(KJob *);
    void onFolderModifyJobFinished(KJob *);
    void onFolderMoveJobFinished(KJob *);
    void onFolderDeleteJobFinished(KJob *);

    void onObjectsRequestJobFinished(KJob *);
    void onObjectsRequestDeltaJobFinished(KJob *);
    void onObjectRequestJobFinished(KJob *);
    void onObjectCreateJobFinished(KJob *);
    void onObjectModifyJobFinished(KJob *);
    void onObjectMoveJobFinished(KJob *);
    void onObjectDeleteJobFinished(KJob *);

    void onFetchResourceCollectionsFinished(KJob *);

private:
    void syncCollectionsRemoteIdCache();
    QMap<qlonglong, Akonadi::Collection> mCollectionsMap;

    Akonadi::Collection mResourceCollection;
    QMap<qlonglong, Akonadi::Collection> mStandardCollectionsMap;

    bool mUseIncrementalUpdates = false;
};

#endif
