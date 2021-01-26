/*
    This file is part of oxaccess.

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef OXA_OBJECTDELETEJOB_H
#define OXA_OBJECTDELETEJOB_H

#include "object.h"

#include <KJob>

namespace OXA
{
class ObjectDeleteJob : public KJob
{
    Q_OBJECT

public:
    explicit ObjectDeleteJob(const Object &object, QObject *parent = nullptr);

    void start() override;

private Q_SLOTS:
    void davJobFinished(KJob *);

private:
    Object mObject;
};
}

#endif
