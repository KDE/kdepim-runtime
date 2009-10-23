/******************************************************************************
 *
 *  File : agent.h
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

#ifndef _FILTERAGENT_H_
#define _FILTERAGENT_H_

#include <akonadi/preprocessorbase.h>
#include <akonadi/collection.h>

#include <akonadi/filter/componentfactoryrfc822.h>
#include <akonadi/filter/agent.h>

#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

#include "filterengine.h"
#include "filterjob.h"

#include <QObject>

namespace Akonadi
{
  namespace Filter
  {
    class ErrorStack;
  } // namespace Filter
} // namespace Akonadi

class FilterAgent : public Akonadi::PreprocessorBase
{
  Q_OBJECT
  Q_CLASSINFO( "D-Bus Interface", "org.freedesktop.Akonadi.FilterAgent" )
public:
  FilterAgent( const QString &id );
  virtual ~FilterAgent();

protected:

  /**
   * The unique instance of the filtering agent.
   */
  static FilterAgent * mInstance;

  /**
   * The hash table of the filter component factories indicized by mimetype.
   */
  QHash< QString, Akonadi::Filter::ComponentFactory * > mComponentFactories;

  /**
   * The hash table of the currently active filtering engines, indicized by engine id.
   * The hash contains owned pointers.
   */
  QHash< QString, FilterEngine * > mEngines;

  /**
   * The hash table of the currently active filter chains indicized by collection id.
   * Each filtered collection has a list of filtering engines (the filtering chain).
   * The engines obviously match the mimetype of the collection.
   * The pointers to the engines are are shallow, the pointers to the lists are owned.
   *
   * Note that we *could* be using a QMultiHash here, but we don't use it
   * because we want to have control of the engine order.
   */
  QHash< Akonadi::Collection::Id, QList< FilterEngine * > * > mFilterChains;

  /**
   * The queue of filtering jobs. Owned pointers.
   */
  QList< FilterJob * > mJobQueue;

  /**
   * The single shot timer we're using to start a job.
   */
  QTimer * mJobStartTimer;

  /**
   * Our busy flag. We set this when running a job so
   * an eventual event-look re-entrancy does not start another job in the meantime.
   */
  bool mBusy;

public:

  /**
   * Returns the unique instance of the filtering agent.
   */
  static FilterAgent * instance()
  {
    return mInstance;
  }

public Q_SLOTS: // D-BUS Interface

  /**
   * Returns the list of the mimetypes that this filtering agent can handle.
   * This method can fail only at D-BUS level: it will never fail at agent level
   * and thus has no returned error conditions.
   *
   * This is a D-BUS method handler.
   */
  QStringList enumerateMimeTypes();

  /**
   * Returns the list of currently existing filter ids that match the specified mimetype.
   * If the mimetype is a null string then the list of filters matching any mimetype is returned.
   * If the mimetyps is invalid then you'll simply get an empty filter list.
   *
   * This is a D-BUS method handler.
   */
  QStringList enumerateFilters( const QString &mimeType );

  /**
   * Creates a new filter with the specified id, mimetype and filter source in sieve format.
   * The id must be an unique non-empty string. If the filter with the specified id already
   * exists then this method fails.
   *
   * This is a D-BUS method handler.
   *
   * If the method succeeds at D-BUS level (so you get a non-error reply)
   * then the result is returned as a member of the Akonadi::Filter::Agent::Status enumeration:
   * Success on success and an Error* constant in case of failure.
   */
  int createFilter( const QString &filterId, const QString &mimeType, const QString &source );

  /**
   * Detaches the filter with the specified id and destroys it.
   * The source file is not touched.
   *
   * This is a D-BUS method handler.
   *
   * If the method succeeds at D-BUS level (so you get a non-error reply)
   * then the result is returned as a member of the Akonadi::Filter::Agent::Status enumeration:
   * Success on success and an Error* constant in case of failure.
   */
  int deleteFilter( const QString &filterId );

  /**
   * Attaches the filter with the specified id to the collections specified
   * by the attachedCollectionsIds list. If the filter is already attached to one
   * of the specified collections then no error is triggered. If one of the collections
   * is invalid then the other collections are processed as usual but an error is returned
   * at the end of the call (you'll get Akonadi::Filter::Agent::ErrorNotAllCollectionsProcessed)
   *
   * If you want the filter to be attached to the specified collections only then preceeded
   * this call with a call to detachFilter().
   *
   * This is a D-BUS method handler.
   *
   * If the method succeeds at D-BUS level (so you get a non-error reply)
   * then the result is returned as a member of the Akonadi::Filter::Agent::Status enumeration:
   * Success on success and an Error* constant in case of failure.
   */
  int attachFilter( const QString &filterId, const QList< Akonadi::Collection::Id > &attachedCollectionIds );

  /**
   * Detaches the filter with the specified id from the specified collections.
   * If you pass an empty list of collections then the filter is completly detached.
   * If the filter wasn't attached to some of the specified collections then
   * the processing will continue with the others but you'll get Akonadi::Filter::Agent::ErrorNotAllCollectionsProcessed
   * as return value.
   *
   * This is a D-BUS method handler.
   *
   * If the method succeeds at D-BUS level (so you get a non-error reply)
   * then the result is returned as a member of the Akonadi::Filter::Agent::Status enumeration:
   * Success on success and an Error* constant in case of failure.
   */
  int detachFilter( const QString &filterId, const QList< Akonadi::Collection::Id > &detachedCollectionIds );

  /**
   * Atomically returns the main properties of a filter: the mimeType, the source
   * of the filtering program and the list of filtered collections.
   *
   * This is a D-BUS method handler.
   *
   * If the method succeeds at D-BUS level (so you get a non-error reply)
   * then the result is returned as a member of the Akonadi::Filter::Agent::Status enumeration:
   * Success on success and an Error* constant in case of failure.
   */
  int getFilterProperties( const QString &filterId, QString &mimeType, QString &source, QList< Akonadi::Collection::Id > &attachedCollectionIds );

  /**
   * Atomically changes the source of the filter and the attached collections.
   * You can't change the mimetype of a filter: you must destroy and re-create it.
   * This call is more or less equivalent to the sequence of deleteFilter(), createFilter() and attachFilter().
   *
   * On success true is returned and a success reply is sent through D-BUS.
   * On failure false is returned and an error message is sent through D-BUS.
   *
   * This is a D-BUS method handler.
   *
   * If the method succeeds at D-BUS level (so you get a non-error reply)
   * then the result is returned as a member of the Akonadi::Filter::Agent::Status enumeration:
   * Success on success and an Error* constant in case of failure.
   */
  int changeFilter( const QString &filterId, const QString &source, const QList< Akonadi::Collection::Id > &attachedCollectionIds );

  /**
   * Queue an explicit request for the application of a filter to a number of Akonadi Items. The request
   * is assigned an unique identifier which is returned in the allocatedJobId parameter. The dbus interface
   * will emit the jobTerminated() signal once the filter has been applied to all the requested items.
   *
   * This is a D-BUS method handler.
   *
   * If the method succeeds at D-BUS level (so you get a non-error reply)
   * then the result is returned as a member of the Akonadi::Filter::Agent::Status enumeration:
   * Success on success and an Error* constant in case of failure.
   */
  int applyFilterToItems( const QString &filterId, const QList< QVariant > &itemIds, qlonglong &allocatedJobId );

Q_SIGNALS:

  /**
   * Emitted to signal the termination of a job triggered by a call to applyFilterToItems().
   *
   * @param jobId The unique identifier of the job that terminated. 
   * @param status An Akonadi::Filter::Agent::Status code
   * @param errorStack An error stack describing error details if status is not Success
   *              and an empty string otherwise.
   */
  void jobTerminated( qlonglong jobId, int status, const Akonadi::Filter::ErrorStack &errorStack );

protected:

  /**
   * Reimplemented from PreprocessorBase in order to handle item processing.
   */
  virtual ProcessingResult processItem( const Akonadi::Item &item );

  /**
   * Reimplemented from AgentBase: launches the filtering console.
   */
  virtual void configure( WId windowId );

private Q_SLOTS:

  void slotRunOneJob();
  void slotAbortRequested();

private:
  Akonadi::Filter::Agent::Status runApplyFilterChainByCollectionJob( FilterJob * job );
  Akonadi::Filter::Agent::Status runApplySpecificFilterJob( FilterJob * job, Akonadi::Filter::ErrorStack &errorStack );

  int createFilterInternal( const QString &filterId, const QString &mimeType, const QString &source, bool saveConfiguration );
  int attachFilterInternal( const QString &filterId, const QList< Akonadi::Collection::Id > &attachedCollectionIds, bool saveConfiguration );

  bool saveFilterSource( const QString &filterId, const QString &source );

  QString fileNameForFilterId( const QString &filterId );

  void detachEngine( FilterEngine * engine );
  bool attachEngine( FilterEngine * engine, Akonadi::Collection::Id collectionId );

  void loadFilterMappings();
  void saveFilterMappings();
};

#endif //!_FILTERAGENT_H_
