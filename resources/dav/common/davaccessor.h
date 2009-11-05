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

#include <QMap>
#include <QSet>

#include <kurl.h>

namespace KIO {
  class DavJob;
  class StoredTransferJob;
}

class KJob;
class QDomDocument;

struct davItem
{
  davItem();
  davItem( const QString &_url, const QString &_contentType, const QByteArray &_data);
  QString url;
  QString contentType;
  QByteArray data;
};

QDataStream& operator<<( QDataStream &out, const davItem &item );
QDataStream& operator>>( QDataStream &in, davItem &item);

enum davItemCacheStatus {
  NOT_CACHED,
  EXPIRED,
  CACHED
};

class davAccessor : public QObject
{
  Q_OBJECT
  
  public:
    davAccessor();
    virtual ~davAccessor();
    virtual void retrieveCollections( const KUrl &url ) = 0;
    virtual void retrieveItems( const KUrl &url ) = 0;
    virtual void retrieveItem( const KUrl &url ) = 0;
    virtual void putItem( const KUrl &url, const QString &contentType, const QByteArray &data, bool useCachedEtag = false );
    virtual void removeItem( const KUrl &url );
    void saveCache();
    
  public Q_SLOTS:
    void validateCache();
    
  protected:
    KIO::DavJob* doPropfind( const KUrl &url, const QDomDocument &properties, const QString &davDepth );
    KIO::DavJob* doReport( const KUrl &url, const QDomDocument &report, const QString &davDepth );
    
    davItemCacheStatus itemCacheStatus( const QString &url, const QString &etag );
    davItem getItemFromCache( const QString &url );
    void addItemToCache( const davItem &item, const QString &etag );
    
    void clearSeenUrls( const QString &url );
    void seenUrl( const QString &collectionUrl, const QString &itemUrl );
    QString getEtagFromHeaders( const QString &httpHeaders );
    
  private Q_SLOTS:
    void itemPutFinished( KJob *j );
    void itemDelFinished( KJob *j );
    void jobWarning( KJob*, const QString&, const QString& );
    
  private:
    QMap<QString, QSet<QString> > lastSeenItems; // collection url, items url
    QMap<QString, QString> etagsCache; // url, etag
    QMap<QString, davItem> itemsCache; // url, item
    
  Q_SIGNALS:
    void accessorStatus( const QString &s );
    void accessorError( const QString &e, bool cancelRequest );
    void collectionRetrieved( const QString &url, const QString &name );
    void collectionsRetrieved();
    void itemRetrieved( const davItem &item );
    void itemsRetrieved();
    void itemPut( const KUrl &oldUrl, const KUrl &newUrl );
    void itemRemoved();
    void backendItemChanged( const davItem &item );
    void backendItemsRemoved( const QList<davItem> &items );
};

#endif