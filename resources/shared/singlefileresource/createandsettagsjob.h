/*
 *  SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */
#ifndef CREATEANDSETTAGSJOB_H
#define CREATEANDSETTAGSJOB_H

#include <KJob>
#include <item.h>
#include <tag.h>
#include "akonadi-singlefileresource_export.h"

class AKONADI_SINGLEFILERESOURCE_EXPORT CreateAndSetTagsJob : public KJob
{
    Q_OBJECT
public:
    explicit CreateAndSetTagsJob(const Akonadi::Item &item, const Akonadi::Tag::List &tags, QObject *parent = nullptr);

    void start() override;

private Q_SLOTS:
    void onCreateDone(KJob *);
    void onModifyDone(KJob *);

private:
    Akonadi::Item mItem;
    Akonadi::Tag::List mTags;
    Akonadi::Tag::List mCreatedTags;
    int mCount;
};

#endif
