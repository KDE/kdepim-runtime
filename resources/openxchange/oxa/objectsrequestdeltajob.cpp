/*
    This file is part of oxaccess.

    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "objectsrequestdeltajob.h"

#include "objectsrequestjob.h"

#include <QtCore/QDebug>

using namespace OXA;

ObjectsRequestDeltaJob::ObjectsRequestDeltaJob( const Folder &folder, qulonglong lastSync, QObject *parent )
  : KJob( parent ), mFolder( folder ), mLastSync( lastSync ), mJobFinishedCount( 0 )
{
}

void ObjectsRequestDeltaJob::start()
{
  ObjectsRequestJob *modifiedJob = new ObjectsRequestJob( mFolder, mLastSync, ObjectsRequestJob::Modified, this );
  connect( modifiedJob, SIGNAL(result(KJob*)), SLOT(fetchModifiedJobFinished(KJob*)) );
  modifiedJob->start();

  ObjectsRequestJob *deletedJob = new ObjectsRequestJob( mFolder, mLastSync, ObjectsRequestJob::Deleted, this );
  connect( deletedJob, SIGNAL(result(KJob*)), SLOT(fetchDeletedJobFinished(KJob*)) );
  deletedJob->start();
}

Object::List ObjectsRequestDeltaJob::modifiedObjects() const
{
  return mModifiedObjects;
}

Object::List ObjectsRequestDeltaJob::deletedObjects() const
{
  return mDeletedObjects;
}

void ObjectsRequestDeltaJob::fetchModifiedJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  const ObjectsRequestJob *requestJob = qobject_cast<ObjectsRequestJob*>( job );

  mModifiedObjects << requestJob->objects();

  mJobFinishedCount++;

  if ( mJobFinishedCount == 2 )
    emitResult();
}

void ObjectsRequestDeltaJob::fetchDeletedJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  const ObjectsRequestJob *requestJob = qobject_cast<ObjectsRequestJob*>( job );

  mDeletedObjects << requestJob->objects();

  mJobFinishedCount++;

  if ( mJobFinishedCount == 2 )
    emitResult();
}

#include "objectsrequestdeltajob.moc"
