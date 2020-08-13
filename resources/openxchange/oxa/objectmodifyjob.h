/*
    This file is part of oxaccess.

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef OXA_OBJECTMODIFYJOB_H
#define OXA_OBJECTMODIFYJOB_H

#include <KJob>

#include "object.h"

namespace OXA {
class ObjectModifyJob : public KJob
{
    Q_OBJECT

public:
    explicit ObjectModifyJob(const Object &object, QObject *parent = nullptr);

    void start() override;

    Object object() const;

private Q_SLOTS:
    void preloadingJobFinished(KJob *);
    void davJobFinished(KJob *);

private:
    Object mObject;
};
}

#endif
