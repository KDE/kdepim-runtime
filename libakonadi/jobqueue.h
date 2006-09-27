/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef AKONADI_JOBQUEUE_H
#define AKONADI_JOBQUEUE_H

#include <libakonadi/job.h>

namespace Akonadi {

class JobQueuePrivate;

/**
  A job queue. All jobs you put in here are executed sequentially. JobQueue can
  be used as a parent for jobs to share the same connection to the Akonadi
  backend. Do not start enqueued jobs manually with start() (exec() is fine)!
*/
class AKONADI_EXPORT JobQueue : public Job
{
  Q_OBJECT
  public:
    /**
      Create a new job queue.
      @param parent The parent object.
    */
    JobQueue( QObject *parent = 0 );

    /**
      Destroys this object.
    */
    virtual ~JobQueue();

    /**
      Returns true if there are no jobs in the queue.
    */
    bool isEmpty() const;

  protected:
    virtual void doStart();
    virtual void addSubJob( Akonadi::Job *job );

  private:
    // part of the Job API, hide it for JobQueue
    virtual void start();
    // uses a singleshot timer to start the job, makes sure its fully constructed
    void startNext();

  private Q_SLOTS:
    void jobDone( Akonadi::Job* job );
    void slotStartNext();

  private:
    JobQueuePrivate *d;
};

}

#endif
