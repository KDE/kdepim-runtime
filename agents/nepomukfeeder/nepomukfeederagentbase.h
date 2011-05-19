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

#include "resource.h"
#include <nie.h>

#include <akonadi/agentbase.h>
#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <akonadi/mimetypechecker.h>

#include <nepomuk/query.h>
#include <nepomuk/resourceterm.h>
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
#include <QtCore/QDateTime>
#include <QtCore/QQueue>

namespace NepomukFast
{
  class PersonContact;
}

namespace Akonadi
{
  class Item;
  class ItemFetchScope;
}

namespace Soprano
{
  class NRLModel;
}

namespace Strigi
{
  class IndexManager;
}

class KJob;

/** Shared base class for all Nepomuk feeders. */
class NepomukFeederAgentBase : public Akonadi::AgentBase, public Akonadi::AgentBase::ObserverV2
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
      // FIXME: why isn't that in the ontology?
      const Nepomuk::Query::ComparisonTerm term( QUrl( QLatin1String( "http://www.semanticdesktop.org/ontologies/2007/01/19/nie#dataGraphFor" ) ),
                                                 Nepomuk::Query::ResourceTerm( entity.url() ) );
      Nepomuk::Query::Query query( term );
      query.setQueryFlags( Nepomuk::Query::Query::NoResultRestrictions );
      const QList<Soprano::Node> list = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery(
          query.toSparqlQuery(), Soprano::Query::QueryLanguageSparql ).iterateBindings( 0 ).allNodes();

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

    /** Reimplement to allow more aggressive initial indexing. */
    virtual Akonadi::ItemFetchScope fetchScopeForCollection( const Akonadi::Collection &collection );

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

    /** Finds (or if it doesn't exist creates) a PersonContact object for the given name and address.
        @param found Used to indicate if the contact is already there are was just newly created. In the latter case you might
        want to add additional information you have available for it.
    */
    static NepomukFast::PersonContact findOrCreateContact( const QString &email, const QString &name, const QUrl &graphUri, bool *found = 0 );

    template <typename R, typename E>
    static void setParent( R& res, const E &entity )
    {
      if ( entity.parentCollection().isValid() && entity.parentCollection() != Akonadi::Collection::root() )
        res.addProperty( Vocabulary::NIE::isPartOf(), entity.parentCollection().url() );
    }

    /**
      Enables Strigi support for indexing attachments.
    */
    void setNeedsStrigi( bool enableStrigi );

    /**
      Index the given data with Strigi. Use for e.g. attachments.
     */
    void indexData( const KUrl &url, const QByteArray &data, const QDateTime &mtime = QDateTime::currentDateTime() );

    /**
     * Set the index compatibility level. If the current level is below this, a full re-indexing is performed.
     */
    void setIndexCompatibilityLevel( int level );

    /**
     * Sets whether the 'Only feed when system is idle' functionality shall be used.
     */
    void disableIdleDetection( bool value );

  public slots:
    /** Trigger a complete update of all items. */
    void updateAll();

  signals:
    void fullyIndexed();

  protected:
    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers );
    void itemRemoved(const Akonadi::Item &item);

    void collectionAdded(const Akonadi::Collection& collection, const Akonadi::Collection& parent);
    void collectionChanged(const Akonadi::Collection& collection, const QSet< QByteArray >& partIdentifiers);
    using AgentBase::ObserverV2::collectionChanged;
    void collectionRemoved(const Akonadi::Collection& collection);

    void doSetOnline(bool online);

  private:
    void processNextCollection();

    /**
      Overrides in subclasses to cause re-indexing on startup to only happen
      when the format changes, for example. Base implementation checks the index compatibility level.
    */
    virtual bool needsReIndexing() const;

    void checkOnline();

  private slots:
    void collectionsReceived( const Akonadi::Collection::List &collections );
    void itemHeadersReceived( const Akonadi::Item::List &items );
    void itemsReceived( const Akonadi::Item::List &items );
    void notificationItemsReceived( const Akonadi::Item::List &items );
    void itemFetchResult( KJob* job );

    void selfTest();
    void slotFullyIndexed();
    void systemIdle();
    void systemResumed();

    void processPipeline();

  private:
    QStringList mSupportedMimeTypes;
    Akonadi::MimeTypeChecker mMimeTypeChecker;
    Akonadi::Collection::List mCollectionQueue;
    Akonadi::Collection mCurrentCollection;
    QQueue<Akonadi::Item> mItemPipeline;
    int mTotalAmount, mProcessedAmount, mPendingJobs;
    QTimer mNepomukStartupTimeout;
    QTimer mProcessPipelineTimer;
    Soprano::NRLModel *mNrlModel;
    Strigi::IndexManager *mStrigiIndexManager;
    int mIndexCompatLevel;
    bool mNepomukStartupAttempted;
    bool mInitialUpdateDone;
    bool mNeedsStrigi;
    bool mSelfTestPassed;
    bool mSystemIsIdle;
    bool mIdleDetectionDisabled;
    bool mReIndex;
};

#endif
