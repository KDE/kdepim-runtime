/*
    Copyright (c) 2016 Stefan Stäglich <sstaeglich@kdemail.net>

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

#ifndef TOMBOYITEMUPLOADJOB_H
#define TOMBOYITEMUPLOADJOB_H

#include "tomboyjobbase.h"
#include <AkonadiCore/Item>
#include <KMime/Message>

enum class JobType {
    AddItem,
    ModifyItem,
    DeleteItem
};

class TomboyItemUploadJob : public TomboyJobBase
{
    Q_OBJECT
public:
    TomboyItemUploadJob(const Akonadi::Item &item, JobType jobType, KIO::AccessManager *manager, QObject *parent = nullptr);

    // Returns mSourceItem for post-processing purposes
    Akonadi::Item item() const;

    JobType jobType() const;

    // automatically called by KJob
    void start() override;

private:
    // This will be called once the request is finished.
    void onRequestFinished();
    // Workaround for https://bugreports.qt-project.org/browse/QTBUG-26161 Qt bug
    QString getCurrentISOTime() const;

    Akonadi::Item mSourceItem;
    KMime::Message::Ptr mNoteContent;

    JobType mJobType;

    int mRemoteRevision;
};

#endif // TOMBOYITEMUPLOADJOB_H
