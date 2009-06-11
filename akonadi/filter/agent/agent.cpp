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

#include "agent.h"

#include "filteragentadaptor.h"

#include <akonadi/attributefactory.h>
#include <akonadi/session.h>
#include <akonadi/monitor.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/item.h>

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

bool FilterAgent::createFilter( const QString &id, const QString &mimeType, const QString &sourceFileName )
{
  if( id.isEmpty() )
  {
    sendErrorReply( QDBusError::Failed, i18n("The specified filter id is empty") );
    return false;
  }

  FilterEngine * engine = mEngines.value( id, 0 );
  if( engine )
  {
    sendErrorReply( QDBusError::Failed, i18n("A filter with the specified unique identifier already exists") );
    return false;
  }

  Akonadi::Filter::ComponentFactory * factory = mComponentFactories.value( mimeType, 0 );
  if( !factory )
  {
    sendErrorReply( QDBusError::Failed, i18n("Filtering of the specified mimetype is not supported") );
    return false;
  }

  engine = new FilterEngine( id, mimeType, factory );

  mEngines.insert( id, engine );

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

#include "agent.moc"
