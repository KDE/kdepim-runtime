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

#include "passwordrequester.h"
#include "imapresourcebase.h"
#include "settings.h"
#include "utils.h"
#include "config-imap.h"

#include "gmailpasswordrequester.h"
#include "settingspasswordrequester.h"

PasswordRequester::PasswordRequester(ImapResourceBase *resource, QObject *parent)
    : PasswordRequesterInterface(parent)
    , mImpl(nullptr)
    , mResource(resource)
{
}

PasswordRequester::~PasswordRequester()
{
}

PasswordRequesterInterface *PasswordRequester::requesterImpl()
{
    if (!mImpl || Utils::isGmail(mResource->settings()->imapServer()) != !!qobject_cast<GmailPasswordRequester *>(mImpl)) {
        if (mImpl) {
            mImpl->disconnect(this);
            mImpl->deleteLater();
        }
        if (Utils::isGmail(mResource->settings()->imapServer())) {
            mImpl = new GmailPasswordRequester(mResource, this);
        } else {
            mImpl = new SettingsPasswordRequester(mResource, this);
        }
        connect(mImpl, &PasswordRequesterInterface::done,
                this, &PasswordRequesterInterface::done);
    }

    return mImpl;
}

void PasswordRequester::cancelPasswordRequests()
{
    requesterImpl()->cancelPasswordRequests();
}

void PasswordRequester::requestPassword(RequestType request, const QString &serverError)
{
    requesterImpl()->requestPassword(request, serverError);
}
