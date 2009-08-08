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
#include <akonadi/mimetypechecker.h>

#include <akonadi/filter/io/sievedecoder.h>

#include <QFile>

#include <KDebug>
#include <KLocale>
#include <KMimeType>
#include <KConfig>
#include <KConfigGroup>
#include <KStandardDirs>


FilterAgent * FilterAgent::mInstance = 0;

FilterAgent::FilterAgent( const QString &id )
  : Akonadi::PreprocessorBase( id )
{
  Q_ASSERT( mInstance == 0 ); // must be unique

  Akonadi::Filter::Agent::registerMetaTypes();

  mBusy = false;
  mInstance = this;

  mComponentFactories.insert( QLatin1String( "message/rfc822" ), new Akonadi::Filter::ComponentFactoryRfc822() );
  mComponentFactories.insert( QLatin1String( "message/news" ), new Akonadi::Filter::ComponentFactoryRfc822() );

  new FilterAgentAdaptor( this );

  loadFilterMappings();

  mJobStartTimer = new QTimer( this );
  mJobStartTimer->setSingleShot( true );

  connect( mJobStartTimer, SIGNAL( timeout() ), this, SLOT( slotRunOneJob() ) );

  // Nicely handle abort requests
  connect( this, SIGNAL( abortRequested() ), this, SLOT( slotAbortRequested() ) );

  //AttributeComponentFactory::registerAttribute<MessageThreadingAttribute>();
  kDebug() << "filteragent: ready and waiting..." ;
}

FilterAgent::~FilterAgent()
{
  Q_ASSERT( !mBusy ); // should have exited any local even loop

  slotAbortRequested();

  saveFilterMappings();

  qDeleteAll( mEngines );
  qDeleteAll( mFilterChains );
  qDeleteAll( mJobQueue );

  delete mJobStartTimer;

  mInstance = 0;
}

void FilterAgent::slotAbortRequested()
{
  if( mJobStartTimer->isActive() )
    mJobStartTimer->stop();

  // kill any pending jobs
  foreach( FilterJob * job, mJobQueue )
  {
    switch( job->type() )
    {
      case FilterJob::ApplyFilterChainByCollection:
        // This was a normal preprocessor job from processItem(): we have one at a time of them
        terminateProcessing( ProcessingFailed );
      break;
      case FilterJob::ApplySpecificFilter:
        // This was an "apply filter now" job
        if( job->emitJobTerminated() )
          emit jobTerminated( job->id(), Akonadi::Filter::Agent::ErrorJobAborted );
      break;
      default:
        Q_ASSERT_X( false, __FUNCTION__, "Unhandled FilterJob::Type" );
      break;
    }
  }

  qDeleteAll( mJobQueue );
}

FilterAgent::ProcessingResult FilterAgent::processItem( Akonadi::Item::Id itemId, Akonadi::Collection::Id collectionId, const QString &mimeType )
{
  // find out the filter chain that we need to apply to this item

  kDebug() << "filteragent: processItem(" << itemId << "," << collectionId << ",'" << mimeType << "')";

  QList< FilterEngine * > * filterChain = mFilterChains.value( collectionId, 0 );

  if( !filterChain )
  {
    kDebug() << "filteragent: no filter chain for collection" << collectionId;
    return ProcessingRefused; // no chain for this collection
  }

  Q_ASSERT( filterChain->count() > 0 );

  FilterJob * job = new FilterJob( FilterJob::ApplyFilterChainByCollection, itemId );
  job->setCollectionId( collectionId );
  job->setMimeType( mimeType );

  mJobQueue.append( job );

  if( !mJobStartTimer->isActive() && !mBusy )
    mJobStartTimer->start( 0 );

  return ProcessingDelayed;
}

Akonadi::Filter::Agent::Status FilterAgent::runJob( FilterJob * job )
{
  kDebug() << "filteragent: run job...";

  switch( job->type() )
  {
    case FilterJob::ApplyFilterChainByCollection:
    {
      kDebug() << "filteragent: will be processing item" << job->itemId() << "in collection" << job->collectionId();

      QList< FilterEngine * > * filterChain = mFilterChains.value( job->collectionId(), 0 );

      if( !filterChain )
      {
        kDebug() << "filteragent: filter chain for collection" << job->collectionId() << "is gone...";
        return Akonadi::Filter::Agent::ErrorNoSuchFilter; // no chain for this collection
      }

      Q_ASSERT( filterChain->count() > 0 );

      Akonadi::Item item( job->itemId() );
      Akonadi::Collection collection( job->collectionId() );

      // We don't use MimeTypeChecker since we'd need to create one for each engine
      // and resolve the mimeType multiple times. The KMimeType api is more "direct" here.

      KMimeType::Ptr mimeTypePtr = KMimeType::mimeType( job->mimeType(), KMimeType::ResolveAliases );
      if( mimeTypePtr.isNull() )
      {
        kWarning() << "filteragent: invalid mimetype" << job->mimeType() << "passed to FilterAgent::processItem()";
        return Akonadi::Filter::Agent::ErrorInvalidParameter;
      }

      // apply each filter
      foreach( FilterEngine * engine, *filterChain )
      {
        // Check mimetype first
        if( !mimeTypePtr->is( engine->mimeType() ) )
        {
          kDebug() << "filteragent: item with mimetype '" << job->mimeType() <<
              "' appeared in collection " << job->collectionId() <<
              " but filter engine with id " << engine->id() <<
              " attached to the collection handles mimetype '" << engine->mimeType() <<
              "which doesn't match";
          continue;
        }

        if( !engine->run( item ) )
        {
          if( engine->errorStack().hasErrors() )
          {
            engine->errorStack().dumpErrorMessage(
                i18n(
                    "Execution of filter '%1' failed for item '%2'",
                    engine->nameOrId(),
                    job->itemId()
                  )
              );
            return Akonadi::Filter::Agent::ErrorFilterExecutionFailed;
          }

          break; // no error, but stop processing
        }
      }

      return Akonadi::Filter::Agent::Success;
    }
    break;
    case FilterJob::ApplySpecificFilter:
    {
      kDebug() << "filteragent: will be processing item" << job->itemId() << "and unconditionally applying filter" << job->filterId();

      FilterEngine * engine = mEngines.value( job->filterId(), 0 );
      if( !engine )
      {
        kDebug() << "filteragent: filter engine" << job->filterId() << "is gone..." << job->collectionId();
        return Akonadi::Filter::Agent::ErrorNoSuchFilter; // no such filter
      }

      Akonadi::Item item( job->itemId() );

      if( !engine->run( item ) )
      {
        if( engine->errorStack().hasErrors() )
        {
            engine->errorStack().dumpErrorMessage(
                i18n(
                    "Execution of filter '%1' failed for item '%2'",
                    engine->nameOrId(),
                    job->itemId()
                  )
              );
          return Akonadi::Filter::Agent::ErrorFilterExecutionFailed;
        }
      }

      return Akonadi::Filter::Agent::Success;
    }
    break;
    default:
      Q_ASSERT_X( false, __FUNCTION__, "Unhandled FilterJob::Type" );
    break;
  }

  Q_ASSERT_X( false, __FUNCTION__, "This point should never be reached" );
  return Akonadi::Filter::Agent::Success; // should be never reached though
}

void FilterAgent::slotRunOneJob()
{
  mJobStartTimer->stop();

  if( mJobQueue.isEmpty() )
  {
    kWarning() << "Job start slot triggered with an empty job queue";
    return; // nothing to do
  }

  FilterJob * job = mJobQueue.takeFirst();
  Q_ASSERT( job );

  Q_ASSERT( !mBusy );

  mBusy = true; // protect agains local event loops inside the filters

  Akonadi::Filter::Agent::Status res = runJob( job );

  mBusy = false;

  switch( job->type() )
  {
    case FilterJob::ApplyFilterChainByCollection:
      // This was a normal preprocessor job from processItem(): we have one at a time of them
      terminateProcessing( ( res == Akonadi::Filter::Agent::Success ) ? ProcessingCompleted : ProcessingFailed );
    break;
    case FilterJob::ApplySpecificFilter:
      // This was an "apply filter now" job
    break;
    default:
      Q_ASSERT_X( false, __FUNCTION__, "Unhandled FilterJob::Type" );
    break;
  }

  if( job->emitJobTerminated() )
    emit jobTerminated( job->id(), res );

  delete job;

  if( mJobQueue.count() > 0 )
    mJobStartTimer->start( 0 );
}

QStringList FilterAgent::enumerateFilters( const QString &mimeType )
{
  QStringList ret;

  // FIXME: Check the mimetype!
  Q_UNUSED( mimeType ); // for now

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

QString FilterAgent::fileNameForFilterId( const QString &filterId )
{
  QString fileName = QString::fromLatin1( "filteragent_source_%1.sieve" ).arg( filterId );
  return KStandardDirs::locateLocal( "config", fileName, true );
}

int FilterAgent::createFilter( const QString &filterId, const QString &mimeType, const QString &source )
{
  return createFilterInternal( filterId, mimeType, source, true );
}

bool FilterAgent::saveFilterSource( const QString &filterId, const QString &source )
{
  QString fileName = fileNameForFilterId( filterId );

  QFile f( fileName );

  if( !f.open( QFile::WriteOnly | QFile::Truncate ) )
  {
    kWarning() << "filteragent: failed to open the file" << fileName << "in order to save the source for filter" << filterId;
    return false;
  }

  QByteArray sourceUtf8 = source.toUtf8();

  if( f.write( sourceUtf8 ) != sourceUtf8.length() )
  {
    kWarning() << "filteragent: failed to write the source for filter with id" << filterId << "to file" << fileName << ":" << f.errorString();
    f.close();
    return false;
  }

  f.close();

  return true;
}

int FilterAgent::createFilterInternal( const QString &filterId, const QString &mimeType, const QString &source, bool saveConfiguration )
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

  Akonadi::Filter::Program * program = decoder.run( source.toUtf8() );
  if( !program )
  {
    kDebug() << decoder.errorStack().errorMessage( i18n( "Filter decoding error" ) );
    return Akonadi::Filter::Agent::ErrorFilterSyntaxError;
  }

  if( saveConfiguration )
  {
    if( !saveFilterSource( filterId, source ) )
      return Akonadi::Filter::Agent::ErrorCouldNotSaveFilter;
  }

  // FIXME: Create engines for other mimetypes
  engine = new FilterEngineRfc822( filterId, mimeType, source, program );

  mEngines.insert( filterId, engine );

  kDebug() << "filteragent: created filter" << filterId;

  if( saveConfiguration )
    saveFilterMappings();

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

    if( !filterChain->contains( engine ) )
      continue;

    filterChain->removeOne( engine );

    Q_ASSERT( !filterChain->contains( engine ) );

    kDebug() << "filteragent: detached filter" << engine->id() << "from collection" << it.key();


    if( filterChain->isEmpty() )
      emptyChains.append( it.key() );
  }

  foreach( Akonadi::Collection::Id id, emptyChains )
  {
    filterChain = mFilterChains.value( id, 0 );
    Q_ASSERT( filterChain );
    Q_ASSERT( filterChain->isEmpty() );

    //changeRecorder()->setCollectionMonitored( Akonadi::Collection( id ), false );
   
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

  kDebug() << "filteragent: destroyed filter" << filterId;

  delete engine;

  return Akonadi::Filter::Agent::Success;
}

bool FilterAgent::attachEngine( FilterEngine * engine, Akonadi::Collection::Id collectionId )
{
  Q_ASSERT( engine );

  // First of all fetch the collection: we need to verify that it exists
  // and has the proper mimetype.
  Akonadi::CollectionFetchJob * job = new Akonadi::CollectionFetchJob( Akonadi::Collection( collectionId ), Akonadi::CollectionFetchJob::Base );

  bool collectionIsValid = job->exec();

  // The Akonadi::Job docs say that the job deletes itself...

  if( !collectionIsValid )
  {
    kDebug() << "filteragent: could not fetch collection data in order to attach engine" << engine->id() << "to collection" << collectionId;
    return false;
  }

  Akonadi::Collection::List collections = job->collections();

  if( collections.count() < 1 )
  {
    kDebug() << "filteragent: ugly result while fetching collection data in order to attach engine" << engine->id() << "to collection" << collectionId;
    return false; // HUH ?
  }

  Q_ASSERT( collections.count() == 1 );

  Akonadi::Collection collection = collections[0];

  // Here MimeTypeChecker helps :)
  
  Akonadi::MimeTypeChecker mimeTypeChecker;
  mimeTypeChecker.addWantedMimeType( engine->mimeType() );

  if( !mimeTypeChecker.isWantedCollection( collection ) )
  {
    kDebug() << "filteragent: refusing to attach engine" << engine->id() << "to collection" << collectionId <<
        "since the collection can't contain the mimetype" << engine->mimeType() << "handled by the engine";
    return false;
  }

  QList< FilterEngine * > * chain = mFilterChains.value( collectionId, 0 );
  if( !chain )
  {
    // no such chain, yet: create it

    chain = new QList< FilterEngine * >();
    mFilterChains.insert( collectionId, chain );

    //kDebug() << "filteragent: now monitoring collection" << collectionId;
    //changeRecorder()->setCollectionMonitored( Akonadi::Collection( collectionId ), true );
  }

  Q_ASSERT( chain );

  if( !chain->contains( engine ) )
  {
    // FIXME: Check that engine mimetype matches the collection mimetype ?
    chain->append( engine );
    kDebug() << "filteragent: attached filter" << engine->id() << "to collection" << collectionId;
  } else {
    kDebug() << "filteragent: filter" << engine->id() << "is already attached to collection" << collectionId;    
  }

  return true;
}

int FilterAgent::attachFilter( const QString &filterId, const QList< Akonadi::Collection::Id > &attachedCollectionIds )
{
  return attachFilterInternal( filterId, attachedCollectionIds, true );
}

int FilterAgent::attachFilterInternal( const QString &filterId, const QList< Akonadi::Collection::Id > &attachedCollectionIds, bool saveConfiguration )
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

  if( saveConfiguration )
    saveFilterMappings();

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

      //kDebug() << "filteragent: no longer monitoring collection" << collectionId;
      //changeRecorder()->setCollectionMonitored( Akonadi::Collection( collectionId ), false );
   
      mFilterChains.remove( collectionId );
      delete chain;
    }
  }

  saveFilterMappings();

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

  Akonadi::Filter::Program * program = decoder.run( source.toUtf8() );
  if( !program )
  {
    kDebug() << decoder.errorStack().errorMessage( i18n( "Filter decoding error" ) );
    return Akonadi::Filter::Agent::ErrorFilterSyntaxError;
  }

  if( !saveFilterSource( filterId, source ) )
    return Akonadi::Filter::Agent::ErrorCouldNotSaveFilter;

  engine->setSource( source );
  engine->setProgram( program );

  detachEngine( engine );

  bool gotInvalidCollection = false;

  foreach( Akonadi::Collection::Id collectionId, attachedCollectionIds )
  {
    kDebug() << "Attaching filer engine " << filterId << " to collection" << collectionId;
    if( !attachEngine( engine, collectionId ) )
      gotInvalidCollection = true;
  }

  saveFilterMappings();

  return gotInvalidCollection ? Akonadi::Filter::Agent::ErrorNotAllCollectionsProcessed : Akonadi::Filter::Agent::Success;
}

int FilterAgent::applyFilterToItems( const QString &filterId, const QList< QVariant > &itemIds, qlonglong &allocatedJobId )
{
  Q_UNUSED( filterId );
  Q_UNUSED( itemIds );

  FilterEngine * engine = mEngines.value( filterId, 0 );

  if( !engine )
  {
    kDebug() << "filteragent: no filter engine with id" << filterId;
    return Akonadi::Filter::Agent::ErrorNoSuchFilter;
  }

  bool first = true;

  foreach( QVariant v, itemIds )
  {
    bool ok;
    Akonadi::Item::Id itemId = v.toLongLong( &ok );
    if( !ok )
      return Akonadi::Filter::Agent::ErrorInvalidParameter;

    FilterJob * job = new FilterJob( FilterJob::ApplySpecificFilter, itemId );
    job->setFilterId( filterId );

    if( first )
    {
      first = false;
      allocatedJobId = job->id();
      job->setEmitJobTerminated( true );
    }

    // The "immediate" jobs go before the server requests
    mJobQueue.prepend( job );
  }

  if( !mJobStartTimer->isActive() && !mBusy )
    mJobStartTimer->start( 0 );

  return Akonadi::Filter::Agent::Success;
}

void FilterAgent::loadFilterMappings()
{
  KConfig cfg( QLatin1String( "filteragent_filtersrc" ), KConfig::SimpleConfig );

  KConfigGroup group = cfg.group( QLatin1String( "Filters" ) );

  QStringList engines = group.readEntry( QLatin1String( "EngineNames" ), QStringList() );

  foreach( QString engineId, engines )
  {
    group = cfg.group( engineId );

    QString mimeType = group.readEntry( QLatin1String( "MimeType" ), QString() );
    if( mimeType.isEmpty() )
    {
      kWarning() << "filteragent: mimetype for configured engine" << engineId << "is empty: skipping";
      continue;
    }

    QString fileName = fileNameForFilterId( engineId );
    QFile f( fileName );
    if( !f.open( QFile::ReadOnly ) )
    {
      kWarning() << "filteragent: could not open sieve source file" << fileName << "for configured engine" << engineId << ": skipping";
      continue;
    }

    QTextStream stream( &f );
    QString source = stream.readAll();
    f.close();

    if( source.isEmpty() )
    {
      kWarning() << "filteragent: the source file" << fileName << "for configured engine" << engineId << "is empty: skipping";
      continue;
    }

    if( createFilterInternal( engineId, mimeType, source, false ) != Akonadi::Filter::Agent::Success )
    {
      kWarning() << "filteragent: failed to create the configured engine" << engineId << ": skipping";
      continue;
    }

    QVariantList collections = group.readEntry( QLatin1String( "Collections" ), QVariantList() );

    QList< Akonadi::Collection::Id > ids;
    foreach( QVariant v, collections )
    {
      bool ok;
      Akonadi::Collection::Id id = static_cast< Akonadi::Collection::Id >( v.toLongLong( &ok ) );
      if( !ok )
      {
        kDebug() << "filteragent: could not decode attached collection id for the configured engine" << engineId << ": skipping";
        continue;
      }
      ids.append( id );
    }

    if( ids.isEmpty() )
    {
      kDebug() << "filteragent: the configured engine" << engineId << "isn't attached";
      continue;
    }

    if( attachFilterInternal( engineId, ids, true ) != Akonadi::Filter::Agent::Success )
    {
      kWarning() << "filteragent: got errors while attaching the configured engine" << engineId << ": some collections might not be filtered";
      continue;
    }

  }
}

void FilterAgent::saveFilterMappings()
{
  KConfig cfg( QLatin1String( "filteragent_filtersrc" ), KConfig::SimpleConfig );

  KConfigGroup group = cfg.group( QLatin1String( "Filters" ) );

  group.deleteGroup(); // clear it

  FilterEngine * engine;

  QStringList engines;

  foreach( engine, mEngines )
    engines << engine->id();

  group.writeEntry( QLatin1String( "EngineNames" ), engines );

  foreach( engine, mEngines )
  {
    QHash< Akonadi::Collection::Id, QList< FilterEngine * > * >::Iterator it;

    group = cfg.group( engine->id() );

    group.writeEntry( QLatin1String( "MimeType" ), engine->mimeType() );

    QVariantList collections;
 
    for( it = mFilterChains.begin(); it != mFilterChains.end(); ++it )
    {
      Q_ASSERT( *it );
      if( ( *it )->contains( engine ) )
        collections.append( QVariant( (qlonglong)it.key() ) );
    }

    group.writeEntry( QLatin1String( "Collections" ), collections );
  }

  cfg.sync();
}


/*
void FilterAgent::configure( WId winId )
{
}


void FilterAgent::itemRemoved(const Akonadi::Item & ref)
{
}

void FilterAgent::collectionChanged( const Akonadi::Collection &col )
{
}
*/

