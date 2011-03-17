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

#include "etagcache.h"

#include <akonadi/resourcebase.h>
#include <akonadi/kcal/freebusyproviderbase.h>

class DavFreeBusyHandler;
class KDateTime;

#include <QtCore/QStringList>

class DavGroupwareResource : public Akonadi::ResourceBase,
                             public Akonadi::AgentBase::Observer,
                             public Akonadi::FreeBusyProviderBase
{
  Q_OBJECT

  public:
    DavGroupwareResource( const QString &id );
    ~DavGroupwareResource();

    virtual void collectionRemoved( const Akonadi::Collection &collection );
    virtual void cleanup();

    virtual KDateTime lastCacheUpdate() const;
    virtual void canHandleFreeBusy( const QString &email ) const;
    virtual void retrieveFreeBusy( const QString &email, const KDateTime &start, const KDateTime &end );

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &collection );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

  protected:
    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );
    virtual void aboutToQuit();
    virtual void doSetOnline( bool online );

  private Q_SLOTS:
    void onCollectionRemovedFinished( KJob* );

    void onHandlesFreeBusy( const QString &email, bool handles );
    void onFreeBusyRetrieved( const QString &email, const QString &freeBusy, bool success, const QString &errorText );

    void onRetrieveCollectionsFinished( KJob* );
    void onRetrieveItemsFinished( KJob* );
    void onMultigetFinished( KJob* );
    void onRetrieveItemFinished( KJob* );
    /**
      * Called when a new item has been fetched from the backend.
      *
      * @param job The job that fetched the item
      * @param updatedItem Set to true if the item fetch has been requested
      * by a refresh.
      */
    void onItemFetched( KJob* job, bool isRefresh = false );
    void onItemRefreshed( KJob* job );

    void onItemAddedFinished( KJob* );
    void onItemChangedFinished( KJob* );
    void onItemRemovedFinished( KJob* );

    void onCollectionDiscovered( int protocol, const QString &collectionUrl, const QString &configuredUrl );

  private:
    bool configurationIsValid();

    /**
     * Collections which only support one mime type have an icon indicating what they support.
     */
    static void setCollectionIcon( Akonadi::Collection &collection );

    Akonadi::Collection mDavCollectionRoot;
    EtagCache mEtagCache;
    QStringList mCollectionsWithTemporaryError;
    DavFreeBusyHandler *mFreeBusyHandler;
};

#endif
