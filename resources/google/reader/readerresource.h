/*
    Copyright (C) 2013  Daniel Vrátil <dvratil@redhat.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef GOOGLE_READER_READERRESOURCE_H
#define GOOGLE_READER_READERRESOURCE_H

#include "../common/googleresource.h"

#include <Akonadi/ResourceBase>
#include <Akonadi/AgentBase>

class QDBusPendingCallWatcher;
namespace KRss {
class FeedCollection;
}

class ChangedItemsCache;
class QTimer;

class ReaderResource : public GoogleResource
{
    Q_OBJECT

  public:
    ReaderResource ( const QString& id );
    virtual ~ReaderResource();

  public Q_SLOTS:
    virtual void retrieveCollections();
    virtual bool retrieveItem( const Akonadi::Item& item, const QSet< QByteArray >& parts );
    virtual void retrieveItems( const Akonadi::Collection& collection );
    void slotCacheTimeout();

  protected Q_SLOTS:
    virtual void collectionAdded( const Akonadi::Collection& collection, const Akonadi::Collection& parent );
    virtual void collectionChanged( const Akonadi::Collection& collection, const QSet< QByteArray >& changedAttributes );
    virtual void collectionMoved( const Akonadi::Collection& collection, const Akonadi::Collection& collectionSource, const Akonadi::Collection& collectionDestination );
    virtual void collectionRemoved( const Akonadi::Collection& collection );

    virtual void itemChanged( const Akonadi::Item& item, const QSet< QByteArray >& partIdentifiers );

  protected:
    bool canPerformTask( bool needsToken = false );
    virtual int runConfigurationDialog( WId windowId );
    virtual QList< QUrl > scopes() const;
    virtual GoogleSettings* settings() const;
    virtual void updateResourceName();

  private Q_SLOTS:
    void slotCollectionsRetrieved( KGAPI2::Job *job );
    void slotItemsRetrieved( KGAPI2::Job *job );
    void slotRenameFolderFetchDone( KJob *job );
    void slotEditTokenRetrieved( KGAPI2::Job *job );
    void slotFaviconRetrieved( QDBusPendingCallWatcher *callWatcher );
    void iconChanged( bool success, const QString &host, const QString &iconName );

  private:
    void fetchEditToken();
    void fetchFavicon( KRss::FeedCollection &collection );


    Akonadi::Collection m_rootCollection;
    QMap<QString, Akonadi::Collection> m_collections;
    QMap<QString, Akonadi::Collection> m_pendingIcons;

    QString m_editToken;

    ChangedItemsCache *m_cache;
    QTimer *m_cacheTimer;

    QDBusInterface *m_favicons;
};

#endif // GOOGLE_READER_READERRESOURCE_H
