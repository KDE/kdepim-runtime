/*
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    SPDX-FileContributor: Kevin Ottens <kevin@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "kimap/sessionuiproxy.h"

#include <KIO/AskIgnoreSslErrorsJob>
#include <KMessageBox>

class SessionUiProxy : public KIMAP::SessionUiProxy
{
public:
    bool ignoreSslError(const KSslErrorUiData &errorData) override
    {
        auto job = new KIO::AskIgnoreSslErrorsJob(errorData, KIO::AskIgnoreSslErrorsJob::RecallAndStoreRules);
        job->exec(); // TODO make KIMAP::SessionUiProxy async
        if (job->error()) {
            KMessageBox::error(nullptr, job->errorString());
        }
        return job->ignored();
    }
};
