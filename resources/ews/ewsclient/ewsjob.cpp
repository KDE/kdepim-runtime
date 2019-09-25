/*
    SPDX-FileCopyrightText: 2015-2019 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsjob.h"
#include "ewsclient_debug.h"

EwsJob::EwsJob(QObject *parent)
    : KCompositeJob(parent)
    , m_ewsRespCode(EwsResponseCodeUnknown)
{
}

EwsJob::~EwsJob()
{
}

bool EwsJob::doKill()
{
    Q_FOREACH (KJob *job, subjobs()) {
        job->kill(KJob::Quietly);
    }
    clearSubjobs();

    return true;
}

bool EwsJob::setErrorMsg(const QString &msg, int code)
{
    setError(code);
    setErrorText(msg);
    qCWarningNC(EWSCLI_LOG) << msg;
    return false;
}
