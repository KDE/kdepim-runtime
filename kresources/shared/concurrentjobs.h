/*
    This file is part of kdepim.
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KRES_AKONADI_CONCURRENTJOBS_H
#define KRES_AKONADI_CONCURRENTJOBS_H

#include "itemsavejob.h"

#include <akonadi/collection.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>

#include <QtCore/QtConcurrentRun>

class ItemSaveContext;

template <class JobClass>
class ConcurrentJob
{
  public:
    ConcurrentJob() : mJob( 0 ) {}

    virtual ~ConcurrentJob() {}

    bool exec()
    {
      QFuture<bool> result = QtConcurrent::run( this, &ConcurrentJob::concurrentExec );
      return result.result();
    }

    QString errorString() const
    {
      return mErrorString;
    }

  protected:
    JobClass *mJob;

    QString mErrorString;

  protected:
    virtual void createJob() = 0;

    virtual void handleSuccess() = 0;

  private:
    bool concurrentExec()
    {
      createJob();
      Q_ASSERT( mJob != 0 );

      bool result = mJob->exec();
      if ( result ) {
        handleSuccess();
      } else {
        mErrorString = mJob->errorString();
      }

      delete mJob;
      return result;
    }
};

class ConcurrentCollectionFetchJob : public ConcurrentJob<Akonadi::CollectionFetchJob>
{
  public:
    const ConcurrentCollectionFetchJob *operator->() const;

    Akonadi::Collection::List collections() const;

  protected:
    Akonadi::Collection::List mCollections;

  protected:
    void createJob();

    void handleSuccess();
};

class ConcurrentItemFetchJob : public ConcurrentJob<Akonadi::ItemFetchJob>
{
  public:
    ConcurrentItemFetchJob( const Akonadi::Collection &collection );

    const ConcurrentItemFetchJob *operator->() const;

    Akonadi::Item::List items() const;

  protected:
    const Akonadi::Collection mCollection;
    Akonadi::Item::List mItems;

  protected:
    void createJob();

    void handleSuccess();
};

class ConcurrentItemSaveJob : public ConcurrentJob<ItemSaveJob>
{
  public:
    ConcurrentItemSaveJob( const ItemSaveContext &saveContext );

    const ConcurrentItemSaveJob *operator->() const;

  protected:
    const ItemSaveContext &mSaveContext;

  protected:
    void createJob();

    void handleSuccess();
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
