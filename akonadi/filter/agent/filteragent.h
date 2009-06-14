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

#ifndef _AKONADI_FILTER_AGENT_H_
#define _AKONADI_FILTER_AGENT_H_

#include <akonadi/agentbase.h>
#include <akonadi/collection.h>

#include <akonadi/filter/componentfactory.h>

#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QVariantList>

#include "filterengine.h"

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

  /**
   * Returns the list of the mimetypes that this filtering agent can handle.
   *
   * This is a D-BUS method handler.
   */
  QStringList enumerateMimeTypes();

  /**
   * Returns the list of currently existing filter ids that match the specified mimetype.
   * If the mimetype is a null string then the list of filters matching any mimetype is returned.
   *
   * This is a D-BUS method handler.
   */
  QStringList enumerateFilters( const QString &mimeType );

  /**
   * Creates a new filter with the specified id, mimetype and filter source in sieve format.
   * The id must be an unique non-empty string. If the filter with the specified id already
   * exists then this method fails.
   *
   * On success true is returned and a success reply is sent through D-BUS.
   * On failure false is returned and an error message is sent through D-BUS.
   *
   * This is a D-BUS method handler.
   */
  bool createFilter( const QString &filterId, const QString &mimeType, const QString &source );

  /**
   * Detaches the filter with the specified id and destroys it.
   * The source file is not touched.
   *
   * On success true is returned and a success reply is sent through D-BUS.
   * On failure false is returned and an error message is sent through D-BUS.
   *
   * This is a D-BUS method handler.
   */
  bool deleteFilter( const QString &filterId );

  bool getFilterProperties( const QString &filterId, QString &mimeType, QString &source, QVariantList &attachedCollectionIds );


  bool attachFilter( const QString &filterId, qint64 collectionId );

  bool detachFilter( const QString &filterId, qint64 collectionId );

protected:
  virtual void configure( WId winId );

  virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
/*
  virtual void itemRemoved( const Akonadi::Item &item );
  virtual void collectionChanged( const Akonadi::Collection &collection );
*/

  void loadConfiguration();
  void saveConfiguration();
};

#endif //!_AKONADI_FILTER_AGENT_H_
