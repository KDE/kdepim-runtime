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

#ifndef DAVACCESSOR_H
#define DAVACCESSOR_H

#include "davcollection.h"
#include "davitem.h"

#include <kurl.h>

#include <QtCore/QMap>
#include <QtCore/QMutex>
#include <QtCore/QSet>

namespace KIO {
  class DavJob;
  class StoredTransferJob;
}

class DavProtocolBase;
class KJob;
class QDomDocument;

enum DavItemCacheStatus {
  NOT_CACHED,
  EXPIRED,
  CACHED
};

class DavAccessor : public QObject
{
  Q_OBJECT

  public:
    DavAccessor( DavProtocolBase *implementation );
    ~DavAccessor();

    void retrieveCollections( const KUrl &url );
    void retrieveItems( const KUrl &url );
    void retrieveItem( const KUrl &url );
    void putItem( const KUrl &url, const QString &contentType, const QByteArray &data, bool useCachedEtag = false );
    void removeItem( const KUrl &url );

    void addItemToCache( const DavItem &item );

  public Q_SLOTS:
    void validateCache();

  Q_SIGNALS:
    void accessorStatus( const QString &s );
    void accessorError( const QString &e, bool cancelRequest );
    void collectionRetrieved( const DavCollection &collection );
    void collectionsRetrieved();
    void itemRetrieved( const DavItem &item );
    void itemsRetrieved();
    void itemPut( const KUrl &oldUrl, DavItem putItem );
    void itemRemoved( const KUrl &url );
    void backendItemsRemoved( const DavItem::List &items );

  private Q_SLOTS:
    void collectionsPropfindFinished( KJob *j );
    void itemsPropfindFinished( KJob *j );
    void itemsReportFinished( KJob *j );
    void itemGetFinished( KJob *j );
    void itemsMultigetFinished( KJob *j );
    void itemPutFinished( KJob *j );
    void itemDelFinished( KJob *j );
    void jobWarning( KJob*, const QString&, const QString& );

  private:
    KIO::DavJob* doPropfind( const KUrl &url, const QDomDocument &properties, const QString &davDepth );
    KIO::DavJob* doReport( const KUrl &url, const QDomDocument &report, const QString &davDepth );
    DavItemCacheStatus itemCacheStatus( const QString &url, const QString &etag );
    DavItem getItemFromCache( const QString &url );
    void clearSeenUrls( const QString &url );
    void seenUrl( const QString &collectionUrl, const QString &itemUrl );
    QString getEtagFromHeaders( const QString &httpHeaders );
    void runItemsFetch( const QString &collection );

    DavProtocolBase *mDavImpl;
    QMap<QString, QSet<QString> > mLastSeenItems; // collection url, items url
    QMap<QString, DavItem> mItemsCache; // url, item
    QMap<QString, int> mRunningItemsQueryCount;
    QMap<QString, QStringList> mFetchItemsQueue; // collection url, items urls
    QMutex mFetchItemsQueueMutex;
};

#endif
