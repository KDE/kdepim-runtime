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

#include "opmlparser.h"
#include "rssitemsync.h"

class QEventLoop;

class KRssLocalResource : public Akonadi::ResourceBase,
                           public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    explicit KRssLocalResource( const QString &id );
    ~KRssLocalResource();
    Akonadi::Collection::List buildCollectionTree( const QList<boost::shared_ptr<const ParsedNode> >& listOfNodes, const Akonadi::Collection &parent);
    QString mimeType();
    void writeFeedsToOpml(const QString &path, const QList<boost::shared_ptr<const ParsedNode> >& nodes);

  public Q_SLOTS:
    virtual void configure( WId windowId );    
  signals:
    void quitOrTimeout();

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    void slotLoadingComplete(Syndication::Loader* loader, Syndication::FeedPtr feed, 
			Syndication::ErrorCode status );
    void fetchCollections();
    void fetchCollectionsFinished( KJob *job );
    void slotItemSyncDone( KJob *job );

			
  protected:
    virtual void aboutToQuit();

    virtual void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );
    virtual void collectionChanged( const Akonadi::Collection &collection );
    virtual void collectionRemoved( const Akonadi::Collection &collection );

private:
    Akonadi::CachePolicy m_policy;
    QHash<Syndication::Loader*, Akonadi::Collection> m_collectionByLoader;
    QTimer *m_writeBackTimer;
    QString m_titleOpml;
    RssItemSync *m_syncer;
    QEventLoop* m_quitLoop;
};

#endif
