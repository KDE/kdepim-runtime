/*
    Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>

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

#include "nepomuktagresource.h"

#include <kdebug.h>
#include <klocale.h>
#include <kurl.h>

#include <akonadi/cachepolicy.h>
#include <akonadi/changerecorder.h>
#include <akonadi/linkjob.h>
#include <akonadi/unlinkjob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/collectioncreatejob.h>

#include <nepomuk/tag.h>
#include <nepomuk/resourcemanager.h>
#include <soprano/signalcachemodel.h>
#include <soprano/nao.h>
#include <soprano/rdf.h>

#include <boost/bind.hpp>
#include <algorithm>
#include <iterator>

using namespace Akonadi;

NepomukTagResource::NepomukTagResource( const QString &id )
        : ResourceBase( id ),
        mModel( new Soprano::Util::SignalCacheModel( Nepomuk::ResourceManager::instance()->mainModel() ) )
{
    Nepomuk::ResourceManager::instance()->init();
    changeRecorder()->fetchCollection( true );
    setName( i18n( "Tags" ) );

    m_root.setName( i18n( "Tags" ) );
    m_root.setRemoteId( "nepomuktags" );
    m_root.setContentMimeTypes( QStringList( Collection::mimeType() ) );
    Collection::Rights rights = Collection::CanCreateCollection;
    m_root.setRights( rights );

    CachePolicy policy;
    policy.setInheritFromParent( false );
    policy.setSyncOnDemand( false );
    policy.setIntervalCheckTime( -1 );
    m_root.setCachePolicy( policy );

    connect( mModel.data(), SIGNAL(statementAdded(Soprano::Statement)),
             SLOT(statementAdded(Soprano::Statement)) );
    connect( mModel.data(), SIGNAL(statementRemoved(Soprano::Statement)),
             SLOT(statementRemoved(Soprano::Statement)) );

    m_pendingTagsTimer.setSingleShot( true );
    m_pendingTagsTimer.setInterval( 500 );
    connect( &m_pendingTagsTimer, SIGNAL(timeout()), SLOT(createPendingTagCollections()) );

    synchronize();
}

NepomukTagResource::~NepomukTagResource()
{
}

void NepomukTagResource::retrieveCollections()
{
    m_pendingTagsTimer.stop();
    m_pendingTagUris.clear();

    QHash<QString,Collection> collections;

    QStringList contentTypes;
    contentTypes << "message/rfc822" << Collection::mimeType();

    collections[ "rootfolderunique" ] = m_root;

    QList<Nepomuk::Tag> tags = Nepomuk::Tag::allTags();
    foreach( const Nepomuk::Tag& tag, tags ) {
        kDebug() << "Found Nepomuk Tag:" << tag.genericLabel();
        if ( collections.contains( tag.genericLabel() ) )
            continue;
        const Collection c = collectionFromTag( tag );
        collections[ tag.genericLabel()] = c;
    }
    collectionsRetrieved( collections.values() );
}

void NepomukTagResource::retrieveItems( const Akonadi::Collection & col )
{
    kDebug() << "Requested items for: " << col.remoteId();
    ItemFetchJob *fetch = new ItemFetchJob( col, this );
    connect( fetch, SIGNAL(result(KJob*)), SLOT(slotLocalListResult(KJob*)) );
}

void NepomukTagResource::slotLocalListResult( KJob *job )
{
    Item::List existingMessages = qobject_cast<ItemFetchJob*>( job )->items();

    Item::List taggedMessages;
    const Nepomuk::Tag tag( currentCollection().remoteId() );
    QList<Nepomuk::Resource> list = tag.tagOf();
    foreach( const Nepomuk::Resource& resource, list ) {
        if ( !resource.resourceUri().toString().startsWith( QLatin1String( "akonadi:" ) ) )
            continue;
        kDebug() << "Found message: " << resource.resourceUri();
        taggedMessages << Item::fromUrl( KUrl( resource.resourceUri() ) );
    }

    std::sort( existingMessages.begin(), existingMessages.end(), boost::bind( &Item::id, _1 ) < boost::bind( &Item::id, _2 ) );
    std::sort( taggedMessages.begin(), taggedMessages.end(), boost::bind( &Item::id, _1 ) < boost::bind( &Item::id, _2 ) );

    Item::List itemsToLink, itemsToUnlink;
    std::set_difference( taggedMessages.begin(), taggedMessages.end(),
                         existingMessages.begin(), existingMessages.end(),
                         std::back_inserter( itemsToLink ),
                         boost::bind( &Item::id, _1 ) < boost::bind( &Item::id, _2 ) );
    std::set_difference( existingMessages.begin(), existingMessages.end(),
                         taggedMessages.begin(), taggedMessages.end(),
                         std::back_inserter( itemsToUnlink ),
                         boost::bind( &Item::id, _1 ) < boost::bind( &Item::id, _2 ) );

    if ( !itemsToUnlink.isEmpty() )
      new Akonadi::UnlinkJob( currentCollection(), itemsToUnlink, this );

    if ( !itemsToLink.isEmpty() ) {
      Akonadi::LinkJob* linkJob = new Akonadi::LinkJob( currentCollection(), itemsToLink, this );
      connect( linkJob, SIGNAL(result(KJob*)), this, SLOT(slotLinkResult(KJob*)) );
    } else {
      itemsRetrievalDone();
    }
}

void NepomukTagResource::linkDone( KJob* job )
{
  if ( job->error() )
    kDebug()<<QString("error on linking: %1").arg( job->errorText() );
}

void NepomukTagResource::unlinkDone( KJob* job )
{
  if ( job->error() )
    kDebug()<<QString("error on unlinking: %1").arg( job->errorText() );
}

void NepomukTagResource::slotLinkResult( KJob* job )
{
    kDebug();
    if ( job->error() ) {
        kDebug() << job->errorString();
        emit error( job->errorString() );
    }
    itemsRetrievalDone();
}

void NepomukTagResource::configure( WId )
{
    synchronizeCollectionTree();
    emit configurationDialogAccepted();
}

void NepomukTagResource::itemLinked(const Akonadi::Item& item, const Akonadi::Collection& collection)
{
    Nepomuk::Resource res( item.url() );
    const Nepomuk::Tag tag( collection.remoteId() );
    res.addTag( tag );
    changeProcessed();
}

void NepomukTagResource::itemUnlinked(const Akonadi::Item& item, const Akonadi::Collection& collection)
{
    Nepomuk::Resource res( item.url() );
    QList<Nepomuk::Tag> allTags = res.tags();
    const Nepomuk::Tag tag( collection.remoteId() );
    allTags.removeAll( tag );
    res.setTags( allTags );
    changeProcessed();
}

void NepomukTagResource::collectionAdded( const Collection & collection, const Collection &parent )
{
    Q_UNUSED( parent );
    QString s = collection.name();
    Collection newCollection = collection;

    // taken from kdelibs/nepomuk/core/ui/kmetadatatagwidget.cpp
    // Copyright (C) 2006-2007 Sebastian Trueg <trueg@kde.org>
    // ---
    bool exists = false;
    if ( !s.isEmpty() ) {
        // see if the tag exists
        QList<Nepomuk::Tag> l = Nepomuk::Tag::allTags();
        QListIterator<Nepomuk::Tag> tagIt( l );
        while ( tagIt.hasNext() ) {
            const Nepomuk::Tag& tag = tagIt.next();
            if ( tag.label() == s || tag.identifiers().contains( s ) ) {
                emit warning( i18n( "The tag %1 already exists", s ) );
                newCollection.setRemoteId( tag.resourceUri().toString() );
                exists = true;
            }
        }
        if ( !exists ) {
            Nepomuk::Tag tag( s );
            tag.setLabel( s );
            newCollection.setRemoteId( tag.resourceUri().toString() );
        }
    }
    // ---

    newCollection.setRights( Collection::ReadOnly | Collection::CanDeleteCollection | Collection::CanLinkItem | Collection::CanUnlinkItem );
    changeCommitted( newCollection );
}

void NepomukTagResource::collectionChanged(const Akonadi::Collection& collection, const QSet< QByteArray >& partIdentifiers)
{
  Q_UNUSED( partIdentifiers );
  Nepomuk::Tag tag( collection.remoteId() );
  EntityDisplayAttribute* attr = collection.attribute<EntityDisplayAttribute>();
  if ( attr && !attr->displayName().isEmpty() )
    tag.setLabel( attr->displayName() );
  else
    tag.setLabel( collection.name() );
  if ( attr && !attr->iconName().isEmpty() )
    tag.setSymbols( QStringList() << attr->iconName() );
  changeCommitted( collection );
}

void NepomukTagResource::collectionRemoved(const Akonadi::Collection& collection)
{
    Nepomuk::Tag tag( collection.remoteId() );
    tag.remove();
    changeCommitted( collection );
}


void NepomukTagResource::statementAdded(const Soprano::Statement& statement)
{
  if ( statement.predicate() == Soprano::Vocabulary::NAO::hasTag() ) {
    const Akonadi::Item item = Item::fromUrl( statement.subject().uri() );
    if ( !item.isValid() )
      return;

    Collection tagCollection;
    tagCollection.setRemoteId( statement.object().uri().toString() );

    LinkJob *job = new LinkJob( tagCollection, Item::List() << item, this );
    connect( job, SIGNAL(result(KJob*)), SLOT(linkDone(KJob*)) );
  } else if ( statement.predicate() == Soprano::Vocabulary::RDF::type() && statement.object() == Soprano::Vocabulary::NAO::Tag() ) {
    // we need to delay the actual folder creation a bit, otherwise we will not see the fully created tag yet
    m_pendingTagUris.append( statement.subject().uri() );
    if ( !m_pendingTagsTimer.isActive() )
      m_pendingTagsTimer.start();
  }
}

void NepomukTagResource::statementRemoved(const Soprano::Statement& statement)
{
  if ( statement.predicate() == Soprano::Vocabulary::NAO::hasTag() ) {

    const Akonadi::Item item = Item::fromUrl( statement.subject().uri() );
    if ( !item.isValid() )
      return;

    Collection tagCollection;
    tagCollection.setRemoteId( statement.object().uri().toString() );

    UnlinkJob *job = new UnlinkJob( tagCollection, Item::List() << item, this );
    connect( job, SIGNAL(result(KJob*)), SLOT(unlinkDone(KJob*)) );
  }
}

Collection NepomukTagResource::collectionFromTag(const Nepomuk::Tag& tag)
{
  Collection c;
  c.setName( tag.genericLabel() );
  c.setRemoteId( tag.resourceUri().toString() );
  c.setRights( Collection::ReadOnly | Collection::CanDeleteCollection | Collection::CanLinkItem | Collection::CanUnlinkItem );
  c.setParentCollection( m_root );
  c.setContentMimeTypes( QStringList() << "message/rfc822" );
  if ( !tag.symbols().isEmpty() ) {
    const QString icon = tag.symbols().first();
    EntityDisplayAttribute *attr = c.attribute<EntityDisplayAttribute>( Collection::AddIfMissing );
    attr->setIconName( icon );
  }
  return c;
}

void NepomukTagResource::createPendingTagCollections()
{
  QList<QUrl> stillPendingTagUris;
  foreach ( const QUrl &tagUri, m_pendingTagUris ) {
    const Nepomuk::Tag tag( tagUri );
    kDebug() << tagUri << tag.label();
    if ( tag.label().isEmpty() ) {
      stillPendingTagUris.append( tagUri );
    } else {
      const Collection c = collectionFromTag( tag );
      new CollectionCreateJob( c, this );
    }
  }

  m_pendingTagUris = stillPendingTagUris;
  if ( !stillPendingTagUris.isEmpty() )
    m_pendingTagsTimer.start();
}


AKONADI_RESOURCE_MAIN( NepomukTagResource )

#include "nepomuktagresource.moc"


