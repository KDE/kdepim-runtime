/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EWSFETCHTASKDETAILJOB_H
#define EWSFETCHTASKDETAILJOB_H

#include "ewsfetchitemdetailjob.h"

class EwsFetchTaskDetailJob : public EwsFetchItemDetailJob
{
    Q_OBJECT
public:
    EwsFetchTaskDetailJob(EwsClient &client, QObject *parent, const Akonadi::Collection &collection);
    ~EwsFetchTaskDetailJob() override;

protected:
    void processItems(const QList<EwsGetItemRequest::Response> &responses) override;
};

#endif
