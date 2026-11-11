/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDateTime>
#include <QHash>

#include "ewsfetchitemdetailjob.h"

class EwsFetchCalendarDetailJob : public EwsFetchItemDetailJob
{
    Q_OBJECT
public:
    EwsFetchCalendarDetailJob(EwsClient &client, QObject *parent, const Akonadi::Collection &collection);
    ~EwsFetchCalendarDetailJob() override;

protected:
    void processItems(const EwsGetItemRequest::Response::List &responses) override;
private Q_SLOTS:
    void exceptionItemsFetched(KJob *job);

private:
    struct ExpectedException {
        QDateTime originalStart;
        QString masterUid;
    };

    // What each requested exception is expected to carry, by item id: a response
    // is only the occurrence asked for when its RECURRENCE-ID matches.
    QHash<QString, ExpectedException> mExpectedExceptions;
};
