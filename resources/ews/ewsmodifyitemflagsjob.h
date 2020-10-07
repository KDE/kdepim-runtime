/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EWSMODIFYITEMFLAGSJOB_H
#define EWSMODIFYITEMFLAGSJOB_H

#include <AkonadiCore/Item>
#include <AkonadiCore/Collection>

#include "ewstypes.h"
#include "ewsclient.h"
#include "ewsjob.h"

class EwsModifyItemFlagsJob : public EwsJob
{
    Q_OBJECT
public:
    EwsModifyItemFlagsJob(EwsClient &client, QObject *parent, const Akonadi::Item::List &, const QSet<QByteArray> &addedFlags, const QSet<QByteArray> &removedFlags);
    ~EwsModifyItemFlagsJob() override;

    Akonadi::Item::List items() const
    {
        return mResultItems;
    }

    void start() override;
protected:
    Akonadi::Item::List mItems;
    Akonadi::Item::List mResultItems;
    EwsClient &mClient;
    QSet<QByteArray> mAddedFlags;
    const QSet<QByteArray> mRemovedFlags;
private Q_SLOTS:
    void itemModifyFinished(KJob *job);
};

#endif
