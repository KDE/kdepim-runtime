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

#include <akonadi/collection.h>
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

  mInstance = this;

  Akonadi::Filter::ComponentFactory * f = new Akonadi::Filter::ComponentFactory();
  mComponentFactories.insert( QLatin1String( "message/rfc822" ), f );

  f = new Akonadi::Filter::ComponentFactory();
  mComponentFactories.insert( QLatin1String( "message/news" ), f );

  new FilterAgentAdaptor( this );

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

  // apply each filter
  foreach( FilterEngine * engine, *filterChain )
  {
#if 0
    engine->run( item, collection );
#endif
  }
}

bool FilterAgent::createFilter( const QString &filterId, const QString &mimeType, const QString &source )
{
  if( filterId.isEmpty() )
  {
    sendErrorReply( QDBusError::Failed, i18n( "The specified filter id is empty" ) );
    return false;
  }

  FilterEngine * engine = mEngines.value( filterId, 0 );
  if( engine )
  {
    sendErrorReply( QDBusError::Failed, i18n( "A filter with the specified unique identifier already exists" ) );
    return false;
  }

  Akonadi::Filter::ComponentFactory * factory = mComponentFactories.value( mimeType, 0 );
  if( !factory )
  {
    sendErrorReply( QDBusError::Failed, i18n( "Filtering of the specified mimetype is not supported" ) );
    return false;
  }

  Akonadi::Filter::IO::SieveDecoder decoder( factory );

  Akonadi::Filter::Program * program = decoder.run( source );
  if( !program )
  {
    sendErrorReply( QDBusError::Failed, i18n( "Error reading filter program: %1", decoder.lastError() ) );
    return false;
  }

  // TODO: store the program in a config file

  engine = new FilterEngine( filterId, mimeType, source, program );

  mEngines.insert( filterId, engine );

  return true;
}

bool FilterAgent::deleteFilter( const QString &filterId )
{
  FilterEngine * engine = mEngines.value( filterId, 0 );
  if( !engine )
  {
    sendErrorReply( QDBusError::Failed, i18n("A filter with the specified unique identifier doesnt' exist") );
    return false;
  }

  mEngines.remove( filterId );

  delete engine;
  return true;
}

bool FilterAgent::attachFilter( const QString &filterId, qint64 collectionId )
{
  FilterEngine * engine = mEngines.value( filterId, 0 );
  if( !engine )
  {
    sendErrorReply( QDBusError::Failed, i18n("A filter with the specified unique identifier doesnt' exist") );
    return false;
  }

  // Fixme: check that the collection is valid!

  QList< FilterEngine * > * chain = mFilterChains.value( collectionId, 0 );
  if( !chain )
  {
    // no such chain, yet: need to verify that the collection exists
    Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob( Akonadi::Collection( collectionId ), Akonadi::CollectionFetchJob::Recursive );

    bool collectionIsValid = job->exec();

    delete job;

    if( !collectionIsValid )
    {
      sendErrorReply( QDBusError::Failed, i18n("The specified collection id is not valid") );
      return false;
    }

    chain = new QList< FilterEngine * >();
    mFilterChains.insert( collectionId, chain );
  }

  chain->append( engine );

  return true;
}

bool FilterAgent::detachFilter( const QString &filterId, qint64 collectionId )
{
  FilterEngine * engine = mEngines.value( filterId, 0 );
  if( !engine )
  {
    sendErrorReply( QDBusError::Failed, i18n("A filter with the specified unique identifier doesnt' exist") );
    return false;
  }

  QList< FilterEngine * > * chain = mFilterChains.value( collectionId, 0 );
  if( !chain )
  {
    sendErrorReply( QDBusError::Failed, i18n("The specified filter is not attacched to the specified collection") );
    return false;
  }

  if( !chain->contains( engine ) )
  {
    sendErrorReply( QDBusError::Failed, i18n("The specified filter is not attacched to the specified collection") );
    return false;
  }

  chain->removeOne( engine );

  if( chain->isEmpty() )
  {
    mFilterChains.remove( collectionId );
    delete chain;
  }

  return true;
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

