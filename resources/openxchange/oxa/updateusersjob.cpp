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

#include "updateusersjob.h"

#include "useridrequestjob.h"
#include "users.h"
#include "usersrequestjob.h"

using namespace OXA;

UpdateUsersJob::UpdateUsersJob( QObject *parent )
  : KJob( parent ), mUserIdRequestFinished( false ), mUsersRequestFinished( false )
{
}

void UpdateUsersJob::start()
{
  UserIdRequestJob *userIdJob = new UserIdRequestJob( this );
  connect( userIdJob, SIGNAL(result(KJob*)), SLOT(userIdRequestJobFinished(KJob*)) );

  UsersRequestJob *usersJob = new UsersRequestJob( this );
  connect( usersJob, SIGNAL(result(KJob*)), SLOT(usersRequestJobFinished(KJob*)) );

  userIdJob->start();
  usersJob->start();
}

void UpdateUsersJob::userIdRequestJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
  } else {
    mUserIdRequestFinished = true;

    UserIdRequestJob *requestJob = qobject_cast<UserIdRequestJob*>( job );
    mUserId = requestJob->userId();

    finish();
  }
}

void UpdateUsersJob::usersRequestJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
  } else {
    mUsersRequestFinished = true;

    UsersRequestJob *requestJob = qobject_cast<UsersRequestJob*>( job );
    mUsers = requestJob->users();

    finish();
  }
}

void UpdateUsersJob::finish()
{
  // check if both sub-jobs have finished
  if ( !(mUserIdRequestFinished && mUsersRequestFinished) )
    return;

  if ( error() ) {
    emitResult();
    return;
  }

  Users::self()->setCurrentUserId( mUserId );
  Users::self()->setUsers( mUsers );

  emitResult();
}

#include "updateusersjob.moc"
