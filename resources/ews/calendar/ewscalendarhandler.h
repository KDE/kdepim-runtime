/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "ewsitemhandler.h"

class EwsCalendarHandler : public EwsItemHandler
{
public:
    EwsCalendarHandler();
    ~EwsCalendarHandler() override;

    EwsFetchItemDetailJob *fetchItemDetailJob(EwsClient &client, QObject *parent, const Akonadi::Collection &collection) override;
    void setSeenFlag(Akonadi::Item &item, bool value) override;
    QString mimeType() override;
    bool setItemPayload(Akonadi::Item &item, const EwsItem &ewsItem) override;
    EwsModifyItemJob *modifyItemJob(EwsClient &client, const QList<Akonadi::Item> &items, const QSet<QByteArray> &parts, QObject *parent) override;
    EwsCreateItemJob *
    createItemJob(EwsClient &client, const Akonadi::Item &item, const Akonadi::Collection &collection, EwsTagStore *tagStore, EwsResource *parent) override;
    static EwsItemHandler *factory();
};
