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

#include "foldersrequestdeltajob.h"

#include "foldersrequestjob.h"

#include <QtCore/QDebug>

using namespace OXA;

FoldersRequestDeltaJob::FoldersRequestDeltaJob( qulonglong lastSync, QObject *parent )
  : KJob( parent ), mLastSync( lastSync ), mJobFinishedCount( 0 )
{
}

void FoldersRequestDeltaJob::start()
{
  FoldersRequestJob *modifiedJob = new FoldersRequestJob( mLastSync, FoldersRequestJob::Modified, this );
  connect( modifiedJob, SIGNAL( result( KJob* ) ), SLOT( fetchModifiedJobFinished( KJob* ) ) );
  modifiedJob->start();

  FoldersRequestJob *deletedJob = new FoldersRequestJob( mLastSync, FoldersRequestJob::Deleted, this );
  connect( deletedJob, SIGNAL( result( KJob* ) ), SLOT( fetchDeletedJobFinished( KJob* ) ) );
  deletedJob->start();
}

Folder::List FoldersRequestDeltaJob::modifiedFolders() const
{
  return mModifiedFolders;
}

Folder::List FoldersRequestDeltaJob::deletedFolders() const
{
  return mDeletedFolders;
}

void FoldersRequestDeltaJob::fetchModifiedJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  const FoldersRequestJob *requestJob = qobject_cast<FoldersRequestJob*>( job );

  mModifiedFolders << requestJob->folders();

  mJobFinishedCount++;

  if ( mJobFinishedCount == 2 )
    emitResult();
}

void FoldersRequestDeltaJob::fetchDeletedJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  const FoldersRequestJob *requestJob = qobject_cast<FoldersRequestJob*>( job );

  mDeletedFolders << requestJob->folders();

  mJobFinishedCount++;

  if ( mJobFinishedCount == 2 )
    emitResult();
}

#include "foldersrequestdeltajob.moc"
