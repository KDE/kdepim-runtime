/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "ewsmodifyitemjob.h"

class EwsModifyMailJob : public EwsModifyItemJob
{
    Q_OBJECT
public:
    EwsModifyMailJob(EwsClient &client, const Akonadi::Item::List &items, const QSet<QByteArray> &parts, QObject *parent);
    ~EwsModifyMailJob() override;
    void start() override;
private Q_SLOTS:
    void updateItemFinished(KJob *job);
};

