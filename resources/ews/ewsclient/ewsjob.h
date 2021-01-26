/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EWSJOB_H
#define EWSJOB_H

#include <KCompositeJob>

class EwsJob : public KCompositeJob
{
    Q_OBJECT
public:
    explicit EwsJob(QObject *parent);
    ~EwsJob() override;

protected:
    bool doKill() override;
    bool setErrorMsg(const QString &msg, int code = KJob::UserDefinedError);
};

#endif
