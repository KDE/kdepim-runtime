/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>
                  2008 Sebastian Trueg <trueg@kde.org>
                  2009 Volker Krause <vkrause@kde.org>
                  2011 Christian Mollekopf <chrigi_1@fastmail.fm>

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
#include "dms-copy/datamanagement.h"
#include <KDE/Nepomuk/Vocabulary/NIE>

namespace Nepomuk
{
  class SimpleResource;
  class SimpleResourceGraph;
}

namespace Akonadi
{
  class Item;
  class ItemFetchScope;
}

namespace Strigi
{
  class IndexManager;
}

class KJob;


/** Shared base class for all Nepomuk feeders. 
 *
 * The feeder adds/removes all items to/from nepomuk as long as the items are available in akonadi.
 * When an item changes, it is removed and inserted again, which ensures that all subproperties created by the feeder
 * are also removed (i.e. addresses of a contact). As long as the item is only modified or moved, but not removed completely from
 * the akonadi storage all properties set by other applications remain untouched. When the item is finally removed from akonadi,
 * all related properties, including properties set by other applications are removed.
 * 
 * After setting the appropriate mimetime, 
 * subclasses only have to reimplement updateItem() where they can set the needed properties on the resource using the info from the Akonadi::Item.
 * They should also add a more specific type from one of the appropriate nie:informationElement subtypes.
 * 
 * The Feeders are supposed to represent the akonadi items as both NIE:InformationElement and NIE:DataObject (for the DataObject side ANEO:AkonadiDataObject is used).
 * If higher level representations such as PIMO:Person from the PIMO ontology which map to real world entities are desired, they have to be created separately. 
 * 
 * Every created resource has the following properties:
 * NIE:url: akonadi uri, can be used to retrieve the akonadi item
 * Akonadi::ItemSearchJob::akonadiItemIdUri(): akonadi id, this attribute is used in queriers to restrict the query to only akonadi items (see ItemSearchJob for more information)
 * nfo:isPartOf: collection hierarchy
 * 
 * To use the same resources from an application, i.e. the Nepomuk::Resource api can be used using the Akonadi::Item::url() in the constructor.
 * This will also work if the item is indexed after being used from the application. 
 * 
 * While the feeder keeps ownership of the created NIE:InformationElement/NIE:DataObject resource and will delete it as soon as the item is removed from akonadi,
 * other resource (i.e. a PIMO representation will not be touched by the feeder)
 * 
 * Reindexing:
 * Subclasses can set the indexCompatibilityLevel to issue a full reindexing of all data when the format changed.
 * Increasing the level, issues a reindexing.
 */
class NepomukFeederAgentBase : public Akonadi::AgentBase, public Akonadi::AgentBase::ObserverV2
{
  Q_OBJECT

  public:
    NepomukFeederAgentBase(const QString& id);
    ~NepomukFeederAgentBase();

    /** Add a supported mimetype. */
    void addSupportedMimeType( const QString &mimeType );

    /** Reimplement to do the actual work. 
     *  
     * It is only necessary to add the attributes to @param res,
     * the storing of the resource will happen automatically.
     * If additional resources are needed, the can be added to @param graph.
     * Additionaly created resources are only removed before an update if they are subresources of the @param res.
     * 
     * It is not necessary for the reimplementation to add @param res to @param graph, nor to store @param graph.
     */
    virtual void updateItem( const Akonadi::Item &item, Nepomuk::SimpleResource &res, Nepomuk::SimpleResourceGraph& graph ) = 0;
    /**
     * Sets the label and icon from the EntityDisplayAttribute.
     *
     * Collections are not supposed to have subresources, so they would not be removed on an update.
     */
    virtual void updateCollection( const Akonadi::Collection &collection, Nepomuk::SimpleResource &res, Nepomuk::SimpleResourceGraph& graph );

    /** Reimplement to allow more aggressive initial indexing. */
    virtual Akonadi::ItemFetchScope fetchScopeForCollection( const Akonadi::Collection &collection );

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
    /** Saves the graph, and marks the data as discardable. Use this function to store data created by the feeder */
    void addGraphToNepomuk( const Nepomuk::SimpleResourceGraph &graph );

  private:
    void processNextCollection();

    /** Set the parent collection of the entity @param entity */
    void setParentCollection( const Akonadi::Entity &entity, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph );

    /**
      Overrides in subclasses to cause re-indexing on startup to only happen
      when the format changes, for example. Base implementation checks the index compatibility level.
    */
    virtual bool needsReIndexing() const;

    void checkOnline();
    
    void addCollectionToNepomuk( const Akonadi::Collection &collection );
    void addItemToGraph( const Akonadi::Item &item, Nepomuk::SimpleResourceGraph &graph );

  private slots:
    void collectionsReceived( const Akonadi::Collection::List &collections );
    void itemHeadersReceived( const Akonadi::Item::List &items );
    void itemsReceived( const Akonadi::Item::List &items );
    void notificationItemsReceived( const Akonadi::Item::List &items );
    void itemFetchResult( KJob* job );
    void removeDataResult( KJob* job );
    void jobResult( KJob* job );

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
    int mPendingRemoveDataJobs;
    QTimer mNepomukStartupTimeout;
    QTimer mProcessPipelineTimer;

    Nepomuk::SimpleResourceGraph *mResourceGraph;

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
