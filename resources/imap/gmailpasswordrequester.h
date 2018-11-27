/*
    Copyright (c) 2016 Daniel Vrátil <dvratil@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef GMAILPASSWORDREQUESTER_H_
#define GMAILPASSWORDREQUESTER_H_

#include "passwordrequesterinterface.h"

#include <QPointer>

class ImapResourceBase;

namespace KGAPI2 {
class AccountPromise;
}

class GmailPasswordRequester : public PasswordRequesterInterface
{
    Q_OBJECT
public:
    explicit GmailPasswordRequester(ImapResourceBase *resource, QObject *parent = nullptr);
    ~GmailPasswordRequester();

    void requestPassword(RequestType request, const QString &serverError) override;
    void cancelPasswordRequests() override;

private Q_SLOTS:
    void onTokenRequestFinished(KGAPI2::AccountPromise *promise);

private:
    ImapResourceBase *mResource = nullptr;
    QPointer<KGAPI2::AccountPromise> mPendingPromise;
};

#endif
