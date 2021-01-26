/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2009, 2010 Kevin Krammer <kevin.krammer@gmx.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_FILESTORE_SESSIONIMPLS_P_H
#define AKONADI_FILESTORE_SESSIONIMPLS_P_H

#include "session_p.h"

namespace Akonadi
{
namespace FileStore
{
/**
 */
class FiFoQueueJobSession : public AbstractJobSession
{
    Q_OBJECT

public:
    explicit FiFoQueueJobSession(QObject *parent = nullptr);

    ~FiFoQueueJobSession() override;

    void addJob(Job *job) override;

    void cancelAllJobs() override;

protected:
    void removeJob(Job *job) override;

private:
    class Private;
    Private *const d;
};
}
}

#endif
