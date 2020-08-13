/*
    This file is part of oxaccess.

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef OXA_USERSREQUESTJOB_H
#define OXA_USERSREQUESTJOB_H

#include <KJob>

#include "user.h"

namespace OXA {
class UsersRequestJob : public KJob
{
    Q_OBJECT

public:
    explicit UsersRequestJob(QObject *parent = nullptr);

    void start() override;

    User::List users() const;

private Q_SLOTS:
    void davJobFinished(KJob *);

private:
    User::List mUsers;
};
}

#endif
