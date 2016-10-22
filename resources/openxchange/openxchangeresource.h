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

    void cleanup() Q_DECL_OVERRIDE;

public Q_SLOTS:
    void configure(WId windowId) Q_DECL_OVERRIDE;
    void aboutToQuit() Q_DECL_OVERRIDE;

protected:
    using ResourceBase::retrieveItems; // Suppress -Woverload-virtual

protected Q_SLOTS:
    void retrieveCollections() Q_DECL_OVERRIDE;
    void retrieveItems(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;

protected:
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;
    void itemRemoved(const Akonadi::Item &item) Q_DECL_OVERRIDE;
    virtual void itemMoved(const Akonadi::Item &item, const Akonadi::Collection &collectionSource,
                           const Akonadi::Collection &collectionDestination) Q_DECL_OVERRIDE;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) Q_DECL_OVERRIDE;
    void collectionChanged(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    // do not hide the other variant, use implementation from base class
    // which just forwards to the one above
    using Akonadi::AgentBase::ObserverV2::collectionChanged;
    void collectionRemoved(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    virtual void collectionMoved(const Akonadi::Collection &collection, const Akonadi::Collection &collectionSource,
                                 const Akonadi::Collection &collectionDestination) Q_DECL_OVERRIDE;

private Q_SLOTS:
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
};

#endif
