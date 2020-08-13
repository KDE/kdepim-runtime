/*
    This file is part of oxaccess.

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "updateusersjob.h"

#include "useridrequestjob.h"
#include "users.h"
#include "usersrequestjob.h"

using namespace OXA;

UpdateUsersJob::UpdateUsersJob(QObject *parent)
    : KJob(parent)
    , mUserIdRequestFinished(false)
    , mUsersRequestFinished(false)
    , mUserId(-1)
{
}

void UpdateUsersJob::start()
{
    UserIdRequestJob *userIdJob = new UserIdRequestJob(this);
    connect(userIdJob, &UserIdRequestJob::result, this, &UpdateUsersJob::userIdRequestJobFinished);

    UsersRequestJob *usersJob = new UsersRequestJob(this);
    connect(usersJob, &UsersRequestJob::result, this, &UpdateUsersJob::usersRequestJobFinished);

    userIdJob->start();
    usersJob->start();
}

void UpdateUsersJob::userIdRequestJobFinished(KJob *job)
{
    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
    } else {
        mUserIdRequestFinished = true;

        UserIdRequestJob *requestJob = qobject_cast<UserIdRequestJob *>(job);
        mUserId = requestJob->userId();

        finish();
    }
}

void UpdateUsersJob::usersRequestJobFinished(KJob *job)
{
    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
    } else {
        mUsersRequestFinished = true;

        UsersRequestJob *requestJob = qobject_cast<UsersRequestJob *>(job);
        mUsers = requestJob->users();

        finish();
    }
}

void UpdateUsersJob::finish()
{
    // check if both sub-jobs have finished
    if (!(mUserIdRequestFinished && mUsersRequestFinished)) {
        return;
    }

    if (error()) {
        emitResult();
        return;
    }

    Users::self()->setCurrentUserId(mUserId);
    Users::self()->setUsers(mUsers);

    emitResult();
}
