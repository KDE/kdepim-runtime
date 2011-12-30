/*
 *  Copyright (c) 2012 Grégory Oestreicher <greg@kamago.net>
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef REPLAY_CACHE_H
#define REPLAY_CACHE_H

#include <Akonadi/Item>

#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QString>

class KJob;

class ReplayCache : public QObject
{
  Q_OBJECT

  public:
    enum ReplayType {
      ItemAdded,
      ItemChanged,
      ItemRemoved
    };

    struct ReplayEntry {
      ReplayType type;
      Akonadi::Item::Id id;
      QString url;
      QString etag;

      typedef QList<ReplayEntry> List;
    };

    ReplayCache();

    void addReplayEntry( const QString &collectionUrl, ReplayType type, const Akonadi::Item &item );
    void delReplayEntry( const QString &collectionUrl, Akonadi::Item::Id id );
    bool hasReplayEntries( const QString &collectionUrl ) const;
    ReplayEntry::List replayEntries( const QString &collectionUrl ) const;

    /**
     * Tries to replay all missed changes from the replay cache for
     * the collection with given @p collectionUrl.
     */
    void flush( const QString &collectionUrl );

  signals:
    void etagChanged( const QString &itemUrl, const QString &etag );

  private Q_SLOTS:
    void onItemFetchFinished( KJob *job );
    void onItemAddFinished( KJob *job );
    void onItemChangeFinished( KJob *job );
    void onItemDeleteFinished( KJob *job );

  private:
    QMap<QString, ReplayEntry::List> mReplayEntries;
};

#endif // REPLAY_CACHE_H