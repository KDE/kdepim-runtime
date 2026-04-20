/*
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "passwordrequesterinterface.h"

class ImapResource;

class PasswordRequester : public PasswordRequesterInterface
{
    Q_OBJECT

public:
    explicit PasswordRequester(ImapResource *resource, QObject *parent = nullptr);
    ~PasswordRequester() override;

    void requestPassword(RequestType request, const QString &serverError) override;
    void cancelPasswordRequests() override;

private:
    PasswordRequesterInterface *requesterImpl();
    PasswordRequesterInterface *mImpl = nullptr;
    ImapResource *const mResource;
};
