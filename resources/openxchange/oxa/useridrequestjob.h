/*
    This file is part of oxaccess.

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef OXA_USERIDREQUESTJOB_H
#define OXA_USERIDREQUESTJOB_H

#include <KJob>

namespace OXA {
class UserIdRequestJob : public KJob
{
    Q_OBJECT

public:
    explicit UserIdRequestJob(QObject *parent = nullptr);

    void start() override;

    qlonglong userId() const;

private Q_SLOTS:
    void davJobFinished(KJob *);

private:
    qlonglong mUserId = -1;
};
}

#endif
