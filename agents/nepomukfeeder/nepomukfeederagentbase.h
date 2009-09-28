/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>
                  2008 Sebastian Trueg <trueg@kde.org>
                  2009 Volker Krause <vkrause@kde.org>

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

#ifndef NEPOMUKFEEDERAGENTBASE_H
#define NEPOMUKFEEDERAGENTBASE_H

#include "selectsqarqlbuilder.h"
#include "resource.h"

#include <akonadi/agentbase.h>
#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <akonadi/mimetypechecker.h>

#include <nepomuk/resourcemanager.h>

#include <Soprano/Model>
#include <Soprano/NodeIterator>
#include <Soprano/Query/QueryLanguage>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/NRL>
#define USING_SOPRANO_NRLMODEL_UNSTABLE_API 1
#include <Soprano/NRLModel>

#include <QStringList>
#include <QtCore/QTimer>

namespace Akonadi
{
  class Item;
}

namespace Soprano
{
  class NRLModel;
}

class KJob;

/** Shared base class for all Nepomuk feeders. */
class NepomukFeederAgentBase : public Akonadi::AgentBase, public Akonadi::AgentBase::Observer2
{
  Q_OBJECT

  public:
    NepomukFeederAgentBase(const QString& id);
    ~NepomukFeederAgentBase();

    /** Remove all references to the given item from Nepomuk. */
    template <typename T>
    static void removeEntityFromNepomuk( const T &entity )
    {
      // find the graph that contains our item and delete the complete graph
      SparqlBuilder::BasicGraphPattern graph;
      // FIXME: why isn't that in the ontology?
      // graph.addTriple( "?g", Vocabulary::Nie::dataGraphFor(), item.url() );
      graph.addTriple( "?g", QUrl( "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#dataGraphFor" ), entity.url() );
      SelectSparqlBuilder qb;
      qb.addQueryVariable( "?g" );
      qb.setGraphPattern( graph );
      const QList<Soprano::Node> list = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery( qb.query(),
          Soprano::Query::QueryLanguageSparql ).iterateBindings( 0 ).allNodes();

      foreach ( const Soprano::Node &node, list )
        Nepomuk::ResourceManager::instance()->mainModel()->removeContext( node );
    }

    /** Adds tags to @p resource based on the given string list. */
    static void tagsFromCategories( NepomukFast::Resource &resource, const QStringList &categories );

    /** Add a supported mimetype. */
    void addSupportedMimeType( const QString &mimeType );

    /** Reimplement to do the actual work. */
    virtual void updateItem( const Akonadi::Item &item, const QUrl &graphUri ) = 0;
    virtual void updateCollection( const Akonadi::Collection &collection, const QUrl &graphUri ) = 0;

    /** Create a graph for the given item with we use to mark all information created by the feeder agent. */
    template <typename T>
    QUrl createGraphForEntity( const T &item )
    {
      QUrl metaDataGraphUri;
      const QUrl graphUri = mNrlModel->createGraph( Soprano::Vocabulary::NRL::InstanceBase(), &metaDataGraphUri );

      // remember to which graph the item belongs to (used in search query in removeItemFromNepomuk())
      mNrlModel->addStatement( graphUri,
                              QUrl::fromEncoded( "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#dataGraphFor", QUrl::StrictMode ),
                              item.url(), metaDataGraphUri );

      return graphUri;
    }

  public slots:
    /** Trigger a complete update of all items. */
    void updateAll();

  protected:
    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers );
    void itemRemoved(const Akonadi::Item &item);

    void collectionAdded(const Akonadi::Collection& collection, const Akonadi::Collection& parent);
    void collectionChanged(const Akonadi::Collection& collection, const QSet< QByteArray >& partIdentifiers);
    void collectionRemoved(const Akonadi::Collection& collection);

  private:
    void processNextCollection();

  private slots:
    void collectionsReceived( const Akonadi::Collection::List &collections );
    void itemHeadersReceived( const Akonadi::Item::List &items );
    void itemsReceived( const Akonadi::Item::List &items );
    void itemFetchResult( KJob* job );

    void selfTest();
    void serviceOwnerChanged( const QString &name, const QString &oldOwner, const QString &newOwner );

  private:
    QStringList mSupportedMimeTypes;
    Akonadi::MimeTypeChecker mMimeTypeChecker;
    Akonadi::Collection::List mCollectionQueue;
    Akonadi::Collection mCurrentCollection;
    int mTotalAmount, mProcessedAmount, mPendingJobs;
    QTimer mNepomukStartupTimeout;
    Soprano::NRLModel *mNrlModel;
    bool mNepomukStartupAttempted;
    bool mInitialUpdateDone;
};

#endif
