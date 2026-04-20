/*
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    SPDX-FileContributor: Kevin Ottens <kevin@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "passwordrequesterinterface.h"

class ImapResource;

namespace QKeychain
{
class ReadPasswordJob;
}

class SettingsPasswordRequester : public PasswordRequesterInterface
{
    Q_OBJECT

public:
    explicit SettingsPasswordRequester(ImapResource *resource, QObject *parent = nullptr);
    ~SettingsPasswordRequester() override;

    void requestPassword(RequestType request = StandardRequest, const QString &serverError = QString()) override;
    void cancelPasswordRequests() override;

private Q_SLOTS:
    void askUserInput(const QString &serverError);

private:
    void slotTryAgainClicked();
    void slotOpenSettingsClicked();

    ImapResource *const m_resource;

    QList<QKeychain::ReadPasswordJob *> m_readPasswordJobs;
};
