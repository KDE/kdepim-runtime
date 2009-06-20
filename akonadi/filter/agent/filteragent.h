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

#include <akonadi/agentbase.h>
#include <akonadi/collection.h>

#include <akonadi/filter/componentfactory.h>
#include <akonadi/filter/agent.h>

#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QStringList>

#include "filterengine.h"

#include <QObject>

class FilterAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::Observer
{
  Q_OBJECT
  Q_CLASSINFO( "D-Bus Interface", "org.freedesktop.Akonadi.FilterAgent" )
public:
  FilterAgent( const QString &id );
  ~FilterAgent();

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
  Q_SCRIPTABLE QStringList enumerateMimeTypes();

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

protected:
  virtual void configure( WId winId );

  virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
/*
  virtual void itemRemoved( const Akonadi::Item &item );
  virtual void collectionChanged( const Akonadi::Collection &collection );
*/

  void detachEngine( FilterEngine * engine );
  bool attachEngine( FilterEngine * engine, Akonadi::Collection::Id collectionId );

  void loadConfiguration();
  void saveConfiguration();
};

#endif //!_FILTERAGENT_H_
