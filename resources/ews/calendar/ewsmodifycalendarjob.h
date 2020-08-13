/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EWSMODIFYCALENDARJOB_H
#define EWSMODIFYCALENDARJOB_H

#include "ewsmodifyitemjob.h"

class EwsModifyCalendarJob : public EwsModifyItemJob
{
    Q_OBJECT
public:
    EwsModifyCalendarJob(EwsClient &client, const Akonadi::Item::List &items, const QSet<QByteArray> &parts, QObject *parent);
    ~EwsModifyCalendarJob() override;
    void start() override;
};

#endif
