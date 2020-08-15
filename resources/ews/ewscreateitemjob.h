/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EWSCREATEITEMJOB_H
#define EWSCREATEITEMJOB_H

#include <AkonadiCore/Item>
#include <AkonadiCore/Collection>
#include "ewsjob.h"

class EwsClient;
class EwsItem;
class EwsTagStore;
class EwsResource;

class EwsCreateItemJob : public EwsJob
{
    Q_OBJECT
public:
    EwsCreateItemJob(EwsClient &client, const Akonadi::Item &item, const Akonadi::Collection &collection, EwsTagStore *tagStore, EwsResource *parent);
    ~EwsCreateItemJob() override;

    virtual bool setSend(bool send = true) = 0;

    const Akonadi::Item &item() const;

    void start() override;
private Q_SLOTS:
    void tagSyncFinished(KJob *job);
protected:
    void populateCommonProperties(EwsItem &item);
    virtual void doStart() = 0;

    Akonadi::Item mItem;
    Akonadi::Collection mCollection;
    EwsClient &mClient;
    EwsTagStore *mTagStore = nullptr;
};

#endif
