/*
    SPDX-FileCopyrightText: 2020 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <AkonadiCore/Item>

#include "ewsclient.h"
#include "ewsjob.h"
#include "ewstypes.h"

class EwsFetchItemPayloadJob : public EwsJob
{
    Q_OBJECT
public:
    EwsFetchItemPayloadJob(EwsClient &client, QObject *parent, const Akonadi::Item::List &items);
    ~EwsFetchItemPayloadJob() override = default;

    Akonadi::Item::List items() const
    {
        return mItems;
    }
    void start() override;
Q_SIGNALS:
    void status(int status, const QString &message = QString());
    void percent(int progress);

private:
    void itemFetchFinished(KJob *job);
    Akonadi::Item::List mItems;
    EwsClient &mClient;
};
