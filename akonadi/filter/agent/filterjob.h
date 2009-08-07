/******************************************************************************
 *
 *  File : filterjob.h
 *  Created on Fri 7 Aug 2009 21:10:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Mail Filtering Agent
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

#ifndef _FILTERJOB_H_
#define _FILTERJOB_H_

#include <QtCore/QString>

#include <akonadi/item.h>
#include <akonadi/collection.h>

/**
 * A descriptor for a single filtering job.
 *
 * At the moment of writing there are two kinds of jobs:
 * a standard akonadi server preprocess request and an
 * "urgent" client (re)process request.
 * See the Type enumeration for more informations about job types.
 *
 * The job has a number of properties depending on its type.
 * The itemId(), collectionId(), filterId() and mimeType()
 * are assigned by the creator of the job while the job id()
 * is automatically generated.
 */
class FilterJob
{
public:
  /**
   * The possible job types.
   */
  enum Type
  {
    /**
     * This is a common Preprocessor job: the akonadi server
     * asks us to filter an item which resides in a specific collection.
     * We find the filter chain attached to this collection and
     * apply it to the item.
     *
     * For this kind of job the valid properties are: id(),  itemId(),
     * collectionId(), emitJobTerminated() and mimeType().
     */
    ApplyFilterChainByCollection,

    /**
     * This is a job requested directly by an application:
     * run filter X on item Y, disregarding its collection.
     * This kind of jobs is treated as "urgent" and they are
     * put first in the queue.
     *
     * For this kind of job the valid properties are: id(),  itemId(),
     * emitJobTerminated() and filterId().
     */
    ApplySpecificFilter
  };

public:

  /**
   * Create a filtering job of a specific type and for a specific item.
   */
  FilterJob( Type type, Akonadi::Item::Id itemId );

  /**
   * Kill the filtering job.
   */
  virtual ~FilterJob();

private:

  /**
   * The unique id of this job. Automatically assigned.
   */
  qint64 mId;

  /**
   * The type of this job. Can't be changed once it's set.
   */
  Type mType;

  /**
   * The id of the filter to apply in this job: valid only
   * for ApplySpecificFilter jobs.
   */
  QString mFilterId;

  /**
   * The id of the item to filter. (Must be) always valid.
   */
  Akonadi::Item::Id mItemId;

  /**
   * The id of the collection that we should use the filtering
   * chain from. Valid only for ApplyFilterChainByCollection jobs.
   */
  Akonadi::Collection::Id mCollectionId;

  /**
   * The mimeType of the item. Valid only for ApplyFilterChainByCollection.
   * The filters in the chain are applied only if they match the mimetype.
   */
  QString mMimeType;

  /**
   * Should we emit the jobCompleted() signal upon job completion ?
   */
  bool mEmitJobTerminated;

public:

  qint64 id() const
  {
    return mId;
  }

  Type type() const
  {
    return mType;
  }

  const QString & filterId() const
  {
    return mFilterId;
  }

  void setFilterId( const QString &filterId )
  {
    mFilterId = filterId;
  }

  Akonadi::Item::Id itemId() const
  {
    return mItemId;
  }

  Akonadi::Collection::Id collectionId() const
  {
    return mCollectionId;
  }

  void setCollectionId( Akonadi::Collection::Id id )
  {
    mCollectionId = id;
  }

  const QString & mimeType() const
  {
    return mMimeType;
  }

  void setMimeType( const QString &mimeType )
  {
    mMimeType = mimeType;
  }

  bool emitJobTerminated()
  {
    return mEmitJobTerminated;
  }

  void setEmitJobTerminated( bool emitIt )
  {
    mEmitJobTerminated = emitIt;
  }

}; // class FilterJob

#endif //!_FILTERJOB_H_
