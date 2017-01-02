/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#ifndef MOVEITEMSTASK_H
#define MOVEITEMSTASK_H

#include "resourcetask.h"

#include <kimap/imapset.h>

class MoveItemsTask : public ResourceTask
{
    Q_OBJECT

public:
    explicit MoveItemsTask(const ResourceStateInterface::Ptr &resource, QObject *parent = nullptr);
    virtual ~MoveItemsTask();

private Q_SLOTS:
    void onSelectDone(KJob *job);
    void onCopyDone(KJob *job);
    void onStoreFlagsDone(KJob *job);
    void onMoveDone(KJob *job);

    void onPreSearchSelectDone(KJob *job);
    void onSearchDone(KJob *job);

protected:
    void doStart(KIMAP::Session *session) Q_DECL_OVERRIDE;

private:
    void startMove(KIMAP::Session *session);
    void triggerMoveJob(KIMAP::Session *session);
    void triggerCopyJob(KIMAP::Session *session);
    void recordNewUid();
    QVector<qint64> imapSetToList(const KIMAP::ImapSet &set);

    KIMAP::ImapSet m_oldSet;
    QVector<qint64> m_newUids;
    QMap<Akonadi::Item::Id /* original ID */, QByteArray> m_messageIds;
};

#endif
