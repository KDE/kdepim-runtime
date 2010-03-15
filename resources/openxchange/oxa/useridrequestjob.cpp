/*
    This file is part of oxaccess.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "useridrequestjob.h"

#include "foldersrequestjob.h"
#include "davmanager.h"

#include <QtCore/QDebug>

using namespace OXA;

UserIdRequestJob::UserIdRequestJob( QObject *parent )
  : KJob( parent ), mUserId( -1 )
{
}

void UserIdRequestJob::start()
{
  FoldersRequestJob *job = new FoldersRequestJob( 0, FoldersRequestJob::Modified, this );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( davJobFinished( KJob* ) ) );

  job->start();
}

qlonglong UserIdRequestJob::userId() const
{
  return mUserId;
}

void UserIdRequestJob::davJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  FoldersRequestJob *requestJob = qobject_cast<FoldersRequestJob*>( job );
  Q_ASSERT( requestJob );

  const Folder::List folders = requestJob->folders();
  foreach ( const Folder &folder, folders ) {
    if ( folder.folderId() == 1 ) {
      // Found folder with 'Private Folders' as parent, so the owner must
      // be the user that is currently logged in.
      mUserId = folder.owner();
      break;
    }
  }

  if ( mUserId == -1 ) {
    setError( UserDefinedError );
    setErrorText( "No private folder found" );
  }

  emitResult();
}

#include "useridrequestjob.moc"
