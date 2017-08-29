/*
    Copyright (c) 2014 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Krammer <kevin.krammer@kdab.com>

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

#ifndef KOLABCHANGEITEMSTAGSTASK_H
#define KOLABCHANGEITEMSTAGSTASK_H

#include "kolabrelationresourcetask.h"
#include "tagchangehelper.h"

class KolabChangeItemsTagsTask : public KolabRelationResourceTask
{
    Q_OBJECT
public:
    explicit KolabChangeItemsTagsTask(const ResourceStateInterface::Ptr &resource, const QSharedPointer<TagConverter> &tagConverter, QObject *parent = nullptr);

protected:
    void startRelationTask(KIMAP::Session *session) override;

private:
    KIMAP::Session *mSession = nullptr;
    QList<Akonadi::Tag> mChangedTags;
    QSharedPointer<TagConverter> mTagConverter;

private:
    void processNextTag();

private Q_SLOTS:
    void onItemsFetchDone(KJob *job);
    void onTagFetchDone(KJob *job);

    void onApplyCollectionChanged(const Akonadi::Collection &collection);
    void onCancelTask(const QString &errorText);
    void onChangeCommitted();
};

#endif // KOLABCHANGEITEMSTAGSTASK_H
