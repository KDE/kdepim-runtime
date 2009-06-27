/******************************************************************************
 *
 *  File : agent.cpp
 *  Created on Sat 16 May 2009 14:24:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi  Filtering Agent
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <pragma@kvirc.net>
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
 *
 *******************************************************************************/

#include "filteragent.h"

#include "filteragentadaptor.h"
#include "filterenginerfc822.h"

#include <akonadi/collection.h>
#include <akonadi/changerecorder.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/item.h>

#include <akonadi/filter/io/sievedecoder.h>

#include <KDebug>
#include <KLocale>


FilterAgent * FilterAgent::mInstance = 0;

FilterAgent::FilterAgent( const QString &id )
  : Akonadi::AgentBase( id )
{
  Q_ASSERT( mInstance == 0 ); // must be unique

  Akonadi::Filter::Agent::registerMetaTypes();

  mInstance = this;

  Akonadi::Filter::ComponentFactory * f = new Akonadi::Filter::ComponentFactory();
  mComponentFactories.insert( QLatin1String( "message/rfc822" ), f );

  f = new Akonadi::Filter::ComponentFactory();
  mComponentFactories.insert( QLatin1String( "message/news" ), f );

  new FilterAgentAdaptor( this );

  // Kill the recorded queue
  while( !changeRecorder()->isEmpty() )
  {
    changeRecorder()->replayNext();
    changeRecorder()->changeProcessed();
  }

  changeRecorder()->setChangeRecordingEnabled( false ); // we want notifications now!


  //AttributeComponentFactory::registerAttribute<MessageThreadingAttribute>();
  kDebug() << "mailfilteragent: ready and waiting..." ;
}

FilterAgent::~FilterAgent()
{
  qDeleteAll( mEngines );
  qDeleteAll( mFilterChains );

  mInstance = 0;
}

void FilterAgent::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  // find out the filter chain that we need to apply to this item
  QList< FilterEngine * > * filterChain = mFilterChains.value( collection.id(), 0 );

  Q_ASSERT( filterChain ); // if this fails then we have received a notification for a collection we shouldn't be watching
  Q_ASSERT( filterChain->count() > 0 );

  kDebug() << "mailfilteragent: item added to collection " << collection.id() ;

  // apply each filter
  foreach( FilterEngine * engine, *filterChain )
  {
    if( !engine->run( item, collection ) )
      break;
  }
}

QStringList FilterAgent::enumerateFilters( const QString &mimeType )
{
  QStringList ret;

  foreach( FilterEngine * engine, mEngines )
    ret.append( engine->id() );

  return ret;
}

QStringList FilterAgent::enumerateMimeTypes()
{
  QStringList ret;
  QList< QString > keys = mComponentFactories.uniqueKeys();

  foreach( QString key, keys )
    ret.append( key );

  return ret;
}

int FilterAgent::createFilter( const QString &filterId, const QString &mimeType, const QString &source )
{
  if( filterId.isEmpty() )
    return Akonadi::Filter::Agent::ErrorInvalidParameter;

  FilterEngine * engine = mEngines.value( filterId, 0 );
  if( engine )
    return Akonadi::Filter::Agent::ErrorFilterAlreadyExists;

  Akonadi::Filter::ComponentFactory * factory = mComponentFactories.value( mimeType, 0 );
  if( !factory )
    return Akonadi::Filter::Agent::ErrorInvalidMimeType;

  if( source.isEmpty() )
    return Akonadi::Filter::Agent::ErrorFilterSyntaxError;

  Akonadi::Filter::IO::SieveDecoder decoder( factory );

  Akonadi::Filter::Program * program = decoder.run( source );
  if( !program )
    return Akonadi::Filter::Agent::ErrorFilterSyntaxError;

  // TODO: store the program in a config file

  engine = new FilterEngineRfc822( filterId, mimeType, source, program );

  mEngines.insert( filterId, engine );

  kDebug() << "mailfilteragent: created filter" << filterId;


  return Akonadi::Filter::Agent::Success;
}

void FilterAgent::detachEngine( FilterEngine * engine )
{
  Q_ASSERT( engine );

  QList< Akonadi::Collection::Id > emptyChains;
  QList< FilterEngine * > * filterChain;

  QHash< Akonadi::Collection::Id, QList< FilterEngine * > * >::Iterator it;

  for( it = mFilterChains.begin(); it != mFilterChains.end(); ++it )
  {
    filterChain = *it;
    Q_ASSERT( filterChain );

    if( filterChain->isEmpty() )
      emptyChains.append( it.key() );
  }

  foreach( Akonadi::Collection::Id id, emptyChains )
  {
    filterChain = mFilterChains.value( id, 0 );
    Q_ASSERT( filterChain );
    Q_ASSERT( filterChain->isEmpty() );

    changeRecorder()->setCollectionMonitored( Akonadi::Collection( id ), false );
   
    mFilterChains.remove( id );
    delete filterChain;
  }
}

int FilterAgent::deleteFilter( const QString &filterId )
{
  FilterEngine * engine = mEngines.value( filterId, 0 );
  if( !engine )
    return Akonadi::Filter::Agent::ErrorNoSuchFilter;

  detachEngine( engine );

  mEngines.remove( filterId );

  kDebug() << "mailfilteragent: destroyed filter" << filterId;

  delete engine;
  return Akonadi::Filter::Agent::Success;
}

bool FilterAgent::attachEngine( FilterEngine * engine, Akonadi::Collection::Id collectionId )
{
  QList< FilterEngine * > * chain = mFilterChains.value( collectionId, 0 );
  if( !chain )
  {
    // no such chain, yet: need to verify that the collection exists
    Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob( Akonadi::Collection( collectionId ), Akonadi::CollectionFetchJob::Recursive );

    bool collectionIsValid = job->exec();

    delete job;

    if( !collectionIsValid )
      return false;

    chain = new QList< FilterEngine * >();
    mFilterChains.insert( collectionId, chain );

    kDebug() << "mailfilteragent: now monitoring collection" << collectionId;

    changeRecorder()->setCollectionMonitored( Akonadi::Collection( collectionId ), true );
  }

  Q_ASSERT( chain );

  if( !chain->contains( engine ) )
  {
    chain->append( engine );
    kDebug() << "mailfilteragent: attached filter" << engine->id() << "to collection" << collectionId;
  }

  return true;
}

int FilterAgent::attachFilter( const QString &filterId, const QList< Akonadi::Collection::Id > &attachedCollectionIds )
{
  FilterEngine * engine = mEngines.value( filterId, 0 );
  if( !engine )
    return Akonadi::Filter::Agent::ErrorNoSuchFilter;

  bool gotInvalidCollection = false;

  foreach( Akonadi::Collection::Id collectionId, attachedCollectionIds )
  {
    if( !attachEngine( engine, collectionId ) )
      gotInvalidCollection = true;
  }

  return gotInvalidCollection ? Akonadi::Filter::Agent::ErrorNotAllCollectionsProcessed : Akonadi::Filter::Agent::Success;
}

int FilterAgent::detachFilter( const QString &filterId, const QList< Akonadi::Collection::Id > &detachedCollectionIds )
{
  FilterEngine * engine = mEngines.value( filterId, 0 );
  if( !engine )
    return Akonadi::Filter::Agent::ErrorNoSuchFilter;

  if( detachedCollectionIds.isEmpty() )
  {
    // detach all
    detachEngine( engine );
    return Akonadi::Filter::Agent::Success;
  }

  bool gotInvalidCollection = false;

  foreach( Akonadi::Collection::Id collectionId, detachedCollectionIds )
  {
    QList< FilterEngine * > * chain = mFilterChains.value( collectionId, 0 );
    if( !chain )
    {
      gotInvalidCollection = true;
      continue;
    }

    if( !chain->contains( engine ) )
    {
      gotInvalidCollection = true;
      continue;
    }

    chain->removeOne( engine );

    if( chain->isEmpty() )
    {

      kDebug() << "mailfilteragent: no longer monitoring collection" << collectionId;

      changeRecorder()->setCollectionMonitored( Akonadi::Collection( collectionId ), false );
   
      mFilterChains.remove( collectionId );
      delete chain;
    }
  }

  return gotInvalidCollection ? Akonadi::Filter::Agent::ErrorNotAllCollectionsProcessed : Akonadi::Filter::Agent::Success;
}

int FilterAgent::getFilterProperties( const QString &filterId, QString &mimeType, QString &source, QList< Akonadi::Collection::Id > &attachedCollectionIds )
{
  FilterEngine * engine = mEngines.value( filterId, 0 );
  if( !engine )
    return Akonadi::Filter::Agent::ErrorNoSuchFilter;

  mimeType = engine->mimeType();
  source = engine->source();
  attachedCollectionIds = mFilterChains.keys();

  return Akonadi::Filter::Agent::Success;
}

int FilterAgent::changeFilter( const QString &filterId, const QString &source, const QList< Akonadi::Collection::Id > &attachedCollectionIds )
{
  FilterEngine * engine = mEngines.value( filterId, 0 );
  if( !engine )
    return Akonadi::Filter::Agent::ErrorNoSuchFilter;

  Akonadi::Filter::ComponentFactory * factory = mComponentFactories.value( engine->mimeType(), 0 );
  Q_ASSERT_X( factory, "FilterAgent::changeFilter", "We got a filter without a corresponding ComponentFactory!" );

  if( source.isEmpty() )
    return Akonadi::Filter::Agent::ErrorFilterSyntaxError;

  Akonadi::Filter::IO::SieveDecoder decoder( factory );

  Akonadi::Filter::Program * program = decoder.run( source );
  if( !program )
    return Akonadi::Filter::Agent::ErrorFilterSyntaxError;

  engine->setSource( source );
  engine->setProgram( program );

  detachEngine( engine );

  bool gotInvalidCollection = false;

  foreach( Akonadi::Collection::Id collectionId, attachedCollectionIds )
  {
    if( !attachEngine( engine, collectionId ) )
      gotInvalidCollection = true;
  }

  return gotInvalidCollection ? Akonadi::Filter::Agent::ErrorNotAllCollectionsProcessed : Akonadi::Filter::Agent::Success;
}

void FilterAgent::loadConfiguration()
{
}

void FilterAgent::saveConfiguration()
{
}

void FilterAgent::configure( WId winId )
{
}

/*
void FilterAgent::itemRemoved(const Akonadi::Item & ref)
{
}

void FilterAgent::collectionChanged( const Akonadi::Collection &col )
{
}
*/

