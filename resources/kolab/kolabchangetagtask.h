/*
    SPDX-FileCopyrightText: 2014 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    SPDX-FileContributor: Kevin Krammer <kevin.krammer@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOLABCHANGETAGTASK_H
#define KOLABCHANGETAGTASK_H

#include "kolabrelationresourcetask.h"
#include "tagchangehelper.h"

class KolabChangeTagTask : public KolabRelationResourceTask
{
    Q_OBJECT
public:
    explicit KolabChangeTagTask(const ResourceStateInterface::Ptr &resource, const QSharedPointer<TagConverter> &tagConverter, QObject *parent = nullptr);

protected:
    void startRelationTask(KIMAP::Session *session) override;

private:
    KIMAP::Session *mSession = nullptr;
    QSharedPointer<TagConverter> mTagConverter;

private Q_SLOTS:
    void onItemsFetchDone(KJob *job);

    void onApplyCollectionChanged(const Akonadi::Collection &collection);
    void onCancelTask(const QString &errorText);
    void onChangeCommitted();
};

#endif // KOLABCHANGETAGTASK_H
