/*
    Copyright (c) 2009 Grégory Oestreicher <greg@kamago.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef DAVGROUPWARERESOURCE_H
#define DAVGROUPWARERESOURCE_H

#include <KDAV/Enums>
#include <KDAV/EtagCache>

#include <memory>

#include <resourcebase.h>
#include <akonadi/calendar/freebusyproviderbase.h>

class DavFreeBusyHandler;

#include <QSet>

namespace KDAV {
class DavItem;
}

class DavGroupwareResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::Observer, public Akonadi::FreeBusyProviderBase
{
    Q_OBJECT

public:
    explicit DavGroupwareResource(const QString &id);
    ~DavGroupwareResource() override;

    void collectionRemoved(const Akonadi::Collection &collection) override;
    void cleanup() override;

    QDateTime lastCacheUpdate() const override;
    void canHandleFreeBusy(const QString &email) const override;
    void retrieveFreeBusy(const QString &email, const QDateTime &start, const QDateTime &end) override;

public Q_SLOTS:
    void configure(WId windowId) override;

private Q_SLOTS:
    void createInitialCache();
    void initialRetrieveCollections();
    void onCreateInitialCacheReady(KJob *);

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
    void collectionChanged(const Akonadi::Collection &collection) override;
    void doSetOnline(bool online) override;

private:
    enum ItemFetchUpdateType {
        ItemUpdateNone,
        ItemUpdateAdd,
        ItemUpdateChange
    };

    KJob *createRetrieveCollectionsJob();
    void onReloadConfig();
    void onCollectionRemovedFinished(KJob *);
    void onCollectionChangedFinished(KJob *job, const Akonadi::Collection &collection);

    void onHandlesFreeBusy(const QString &email, bool handles);
    void onFreeBusyRetrieved(const QString &email, const QString &freeBusy, bool success, const QString &errorText);

    void onRetrieveCollectionsFinished(KJob *);
    void onRetrieveItemsFinished(KJob *);
    void onMultigetFinished(KJob *);
    void onRetrieveItemFinished(KJob *);
    /**
      * Called when a new item has been fetched from the backend.
      *
      * @param job The job that fetched the item
      * @param updateType The type of update that triggered this call. The task notification sent
      *        sent to Akonadi will depend on this flag.
      */
    void onItemFetched(KJob *job, ItemFetchUpdateType updateType);
    void onItemRefreshed(KJob *job);

    void onItemAddedFinished(KJob *);
    void onItemChangePrepared(KJob *);
    void onItemChangedFinished(KJob *);
    void onItemRemovalPrepared(KJob *);
    void onItemRemovedFinished(KJob *);

    void onCollectionDiscovered(KDAV::Protocol protocol, const QString &collectionUrl, const QString &configuredUrl);
    void onConflictModifyJobFinished(KJob *job);
    void onDeletedItemRecreated(KJob *job);

    void doItemChange(const Akonadi::Item &item, const Akonadi::Item::List &dependentItems = Akonadi::Item::List());
    void doItemRemoval(const Akonadi::Item &item);
    void handleConflict(const Akonadi::Item &localItem, const Akonadi::Item::List &localDependentItems, const KDAV::DavItem &remoteItem, bool isLocalRemoval, int responseCode);

    bool configurationIsValid();
    void retryAfterFailure(const QString &errorMessage);

    /**
     * Collections which only support one mime type have an icon indicating what they support.
     */
    static void setCollectionIcon(Akonadi::Collection &collection);

    Akonadi::Collection mDavCollectionRoot;
    QMap<QString, std::shared_ptr<KDAV::EtagCache> > mEtagCaches;
    QMap<QString, QString> mCTagCache;
    DavFreeBusyHandler *mFreeBusyHandler = nullptr;
    bool mSyncErrorNotified = false;
};

#endif
