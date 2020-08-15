/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EWSCREATEABCHPERSONJOB_H
#define EWSCREATEABCHPERSONJOB_H

#include "ewscreateitemjob.h"

class EwsCreateAbchPersonJob : public EwsCreateItemJob
{
    Q_OBJECT
public:
    EwsCreateAbchPersonJob(EwsClient &client, const Akonadi::Item &item, const Akonadi::Collection &collection, EwsTagStore *tagStore, EwsResource *parent);
    ~EwsCreateAbchPersonJob() override;
    bool setSend(bool send = true) override;
protected:
    void doStart() override;
};

#endif
