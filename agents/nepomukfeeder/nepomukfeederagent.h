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

#ifndef NEPOMUKFEEDERAGENT_H
#define NEPOMUKFEEDERAGENT_H

#include <akonadi/agentbase.h>
#include <akonadi/collection.h>
#include <akonadi/item.h>

#include <QtCore/QTimer>
#include "feederqueue.h"

class FeederPluginloader;
namespace Nepomuk
{
  class SimpleResource;
  class SimpleResourceGraph;
}

class KJob;

namespace Akonadi
{
  class Item;
  class ItemFetchScope;


/** Shared base class for all Nepomuk feeders. 
 *
 * The feeder adds/removes all items to/from nepomuk as long as the items are available in akonadi.
 * When an item changes, it is removed and inserted again, which ensures that all subproperties created by the feeder
 * are also removed (i.e. addresses of a contact). As long as the item is only modified or moved, but not removed completely from
 * the akonadi storage all properties set by other applications remain untouched. When the item is finally removed from akonadi,
 * all related properties, including properties set by other applications are removed.
 * 
 * Plugins can subscribe to mimetypes in their desktop files, which ensures they get the chance add their information to the passed Resource
 * 
 * The Feeders are supposed to represent the akonadi items as both NIE:InformationElement and NIE:DataObject (for the DataObject side ANEO:AkonadiDataObject is used).
 * If higher level representations such as PIMO:Person from the PIMO ontology which map to real world entities are desired, they have to be created separately. 
 * 
 * Every created resource has the following properties:
 * NIE:url: akonadi uri, can be used to retrieve the akonadi item
 * ANEO::akonadiItemId: akonadi id, Depreceated usage: this attribute is used in queries to restrict the query to only akonadi items (see ItemSearchJob for more information)
 * nfo:isPartOf: collection hierarchy.
 * ANEO::AkonadiDataObject: Datatype of all resources created by the feeder. Can be used to restrict the query to only akonadi entities.
 * 
 * To use the same resources from an application, i.e. the Nepomuk::Resource api can be used using the Akonadi::Item::url() in the constructor or
 * a SimpleResource with the NIE:url property set to Akonadi::Item::url().
 * Both ways will also work if the item is indexed after being used from the application. 
 * 
 * While the feeder keeps ownership of the created NIE:InformationElement/NIE:DataObject resource and will delete it as soon as the item is removed from akonadi,
 * other resource (i.e. a PIMO representation will not be touched by the feeder)
 * 
 * Reindexing:
 * Increasing the mIndexCompatLevel, issues a reindexing.
 */
class NepomukFeederAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::ObserverV2
{
  Q_OBJECT

  public:
    NepomukFeederAgent(const QString& id);
    ~NepomukFeederAgent();

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

    /**
      Overrides in subclasses to cause re-indexing on startup to only happen
      when the format changes, for example. Base implementation checks the index compatibility level.
    */
    virtual bool needsReIndexing() const;

    void checkOnline();

  private slots:
    void selfTest();
    void slotFullyIndexed();
    void systemIdle();
    void systemResumed();
    void collectionsReceived( const Akonadi::Collection::List &collections );
    void idle(const QString &);
    void running(const QString &);
  private:
    QTimer mNepomukStartupTimeout;

    int mIndexCompatLevel;
    bool mNepomukStartupAttempted;
    bool mInitialUpdateDone;
    bool mSelfTestPassed;
    bool mSystemIsIdle;
    bool mIdleDetectionDisabled;
    
    FeederQueue mQueue;
};

}

#endif
