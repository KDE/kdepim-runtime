/*
    This file is part of oxaccess.

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef OXA_UPDATEUSERSJOB_H
#define OXA_UPDATEUSERSJOB_H

#include <KJob>

#include "user.h"

namespace OXA {
class UpdateUsersJob : public KJob
{
    Q_OBJECT

public:
    explicit UpdateUsersJob(QObject *parent = nullptr);

    void start() override;

private Q_SLOTS:
    void userIdRequestJobFinished(KJob *);
    void usersRequestJobFinished(KJob *);

private:
    void finish();

    bool mUserIdRequestFinished;
    bool mUsersRequestFinished;
    User::List mUsers;
    qlonglong mUserId;
};
}

#endif
