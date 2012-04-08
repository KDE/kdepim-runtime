/*
    Copyright (C) 2011    Alessandro Cosentino <cosenal@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef KRSSLOCALRESOURCE_H
#define KRSSLOCALRESOURCE_H

#include <Akonadi/CachePolicy>
#include <Akonadi/ResourceBase>
#include <boost/shared_ptr.hpp>
#include <Syndication/Syndication>
#include <QTimer>
#include <QHash>

#include "rssitemsync.h"

class QEventLoop;

class KRssLocalResource : public Akonadi::ResourceBase,
                           public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    explicit KRssLocalResource( const QString &id );
    ~KRssLocalResource();

  public Q_SLOTS:
    void configure( WId windowId );

  protected:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

    virtual void aboutToQuit();

    void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );
    void collectionChanged( const Akonadi::Collection &collection );
    void collectionRemoved( const Akonadi::Collection &collection );

  private Q_SLOTS:
    void slotLoadingComplete(Syndication::Loader* loader, Syndication::FeedPtr feed, 
			Syndication::ErrorCode status );
    void startOpmlExport();
    void opmlExportFinished( KJob *job );
    void opmlImportFinished( KJob *job );
    void slotItemSyncDone( KJob *job );

private:
    Akonadi::CachePolicy m_policy;
    QHash<Syndication::Loader*, Akonadi::Collection> m_collectionByLoader;
    QTimer *m_writeBackTimer;
    QString m_titleOpml;
    RssItemSync *m_syncer;
    QEventLoop* m_quitLoop;
};

#endif
