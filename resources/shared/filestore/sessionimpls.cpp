/*  This file is part of the KDE project
    Copyright (C) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "sessionimpls_p.h"

#include "collectioncreatejob.h"
#include "collectiondeletejob.h"
#include "collectionfetchjob.h"
#include "collectionmodifyjob.h"
#include "collectionmovejob.h"
#include "itemcreatejob.h"
#include "itemdeletejob.h"
#include "itemfetchjob.h"
#include "itemmodifyjob.h"
#include "itemmovejob.h"
#include "storecompactjob.h"

#include <KDebug>

#include <QQueue>
#include <QTimer>

using namespace Akonadi;

class AbstractEnqueueVisitor : public FileStore::Job::Visitor
{
  public:
    bool visit( FileStore::Job *job ) {
      enqueue( job, "Job" );
      return true;
    }

    bool visit( FileStore::CollectionCreateJob *job ) {
      enqueue( job, "CollectionCreateJob" );
      return true;
    }

    bool visit( FileStore::CollectionDeleteJob *job ) {
      enqueue( job, "CollectionDeleteJob" );
      return true;
    }

    bool visit( FileStore::CollectionFetchJob *job ) {
      enqueue( job, "CollectionFetchJob" );
      return true;
    }

    bool visit( FileStore::CollectionModifyJob *job ) {
      enqueue( job, "CollectionModifyJob" );
      return true;
    }

    bool visit( FileStore::CollectionMoveJob *job ) {
      enqueue( job, "CollectionMoveJob" );
      return true;
    }

    bool visit( FileStore::ItemCreateJob *job ) {
      enqueue( job, "ItemCreateJob" );
      return true;
    }

    bool visit( FileStore::ItemDeleteJob *job ) {
      enqueue( job, "ItemDeleteJob" );
      return true;
    }

    bool visit( FileStore::ItemFetchJob *job ) {
      enqueue( job, "ItemFetchJob" );
      return true;
    }

    bool visit( FileStore::ItemModifyJob *job ) {
      enqueue( job, "ItemModifyJob" );
      return true;
    }

    bool visit( FileStore::ItemMoveJob *job ) {
      enqueue( job, "ItemMoveJob" );
      return true;
    }

    bool visit( FileStore::StoreCompactJob *job ) {
      enqueue( job, "StoreCompactJob" );
      return true;
    }

  protected:
    virtual void enqueue( FileStore::Job *job, const char* className ) = 0;
};

class FileStore::FiFoQueueJobSession::Private : public AbstractEnqueueVisitor
{
  public:
    explicit Private( FileStore::FiFoQueueJobSession *parent )
      : mParent( parent )
    {
      QObject::connect( &mJobRunTimer, SIGNAL(timeout()), mParent, SLOT(runNextJob()) );
    }

    void runNextJob()
    {
/*      kDebug() << "Queue with" << mJobQueue.count() << "entries";*/
      if ( mJobQueue.isEmpty() ) {
        mJobRunTimer.stop();
        return;
      }

      FileStore::Job *job = mJobQueue.dequeue();
      while ( job != 0 && job->error() != 0 ) {
/*        kDebug() << "Dequeued job" << job << "has error ("
                 << job->error() << "," << job->errorText() << ")";*/
        mParent->emitResult( job );
        if ( !mJobQueue.isEmpty() ) {
          job = mJobQueue.dequeue();
        } else {
          job = 0;
        }
      }

      if ( job != 0 ) {
/*        kDebug() << "Dequeued job" << job << "is ready";*/
        QList<FileStore::Job*> jobs;
        jobs << job;

        emit mParent->jobsReady( jobs );
      } else {
/*        kDebug() << "Queue now empty";*/
        mJobRunTimer.stop();
      }
    }

  public:
    QQueue<FileStore::Job*> mJobQueue;

    QTimer mJobRunTimer;

  protected:
    virtual void enqueue( FileStore::Job *job, const char* className )
    {
      Q_UNUSED( className );
      mJobQueue.enqueue( job );

//       kDebug() << "adding" << className << ". Queue now with"
//                << mJobQueue.count() << "entries";

      mJobRunTimer.start( 0 );
    }

  private:
    FileStore::FiFoQueueJobSession *mParent;
};

FileStore::FiFoQueueJobSession::FiFoQueueJobSession( QObject *parent )
  : FileStore::AbstractJobSession( parent ), d( new Private( this ) )
{
}

FileStore::FiFoQueueJobSession::~FiFoQueueJobSession()
{
  cancelAllJobs();
  delete d;
}

void FileStore::FiFoQueueJobSession::addJob( FileStore::Job *job )
{
  job->accept( d );
}

void FileStore::FiFoQueueJobSession::cancelAllJobs()
{
  // KJob::kill() also deletes the job
  foreach( FileStore::Job *job, d->mJobQueue ) {
    job->kill( KJob::EmitResult );
  }

  d->mJobQueue.clear();
}

void FileStore::FiFoQueueJobSession::removeJob( FileStore::Job *job )
{
  d->mJobQueue.removeAll( job );
}

#include "sessionimpls_p.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
