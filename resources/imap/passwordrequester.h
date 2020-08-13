/*
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PASSWORDREQUESTER_H_
#define PASSWORDREQUESTER_H_

#include "passwordrequesterinterface.h"

class ImapResourceBase;

class PasswordRequester : public PasswordRequesterInterface
{
    Q_OBJECT

public:
    explicit PasswordRequester(ImapResourceBase *resource, QObject *parent = nullptr);
    ~PasswordRequester() override;

    void requestPassword(RequestType request, const QString &serverError) override;
    void cancelPasswordRequests() override;

private:
    PasswordRequesterInterface *requesterImpl();
    PasswordRequesterInterface *mImpl = nullptr;
    ImapResourceBase *mResource = nullptr;
};

#endif
