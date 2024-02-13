/*
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "passwordrequester.h"
#include "imapresourcebase.h"
#include "passwordrequesterinterface.h"
#include "settings.h"
#include <config-imap.h>

#include "gmailpasswordrequester.h"
#include "outlookpasswordrequester.h"
#include "settingspasswordrequester.h"

namespace
{

XOAuthPasswordRequester *createXOAuthPasswordRequester(ImapResourceBase *resource, QObject *parent)
{
    static const auto isGmail = [](QStringView server) {
        return server.endsWith(u".gmail.com") || server.endsWith(u".googlemail.com");
    };
    static const auto isOutlook = [](QStringView server) {
        return server.endsWith(u".outlook.com") || server.endsWith(u".office365.com") || server.endsWith(u".hotmail.com");
    };

    const auto imapServer = resource->settings()->imapServer();
    if (isGmail(imapServer)) {
        return new GmailPasswordRequester(resource, parent);
    }
    if (isOutlook(imapServer)) {
        return new OutlookPasswordRequester(resource, parent);
    }

    return nullptr;
}

}

PasswordRequester::PasswordRequester(ImapResourceBase *resource, QObject *parent)
    : PasswordRequesterInterface(parent)
    , mResource(resource)
{
}

PasswordRequester::~PasswordRequester() = default;

PasswordRequesterInterface *PasswordRequester::requesterImpl()
{
    const bool isXOAuth = mResource->settings()->authentication() == MailTransport::Transport::EnumAuthenticationType::XOAUTH2;
    if (!mImpl || (isXOAuth && (qobject_cast<XOAuthPasswordRequester *>(mImpl) == nullptr))) {
        if (mImpl) {
            mImpl->disconnect(this);
            mImpl->deleteLater();
        }
        if (isXOAuth) {
            mImpl = createXOAuthPasswordRequester(mResource, this);
        }
        if (!isXOAuth || !mImpl) {
            // Fallback to SettingsPasswordRequester even if XOAuth is configured, but we don't support
            // the provider as of now.
            mImpl = new SettingsPasswordRequester(mResource, this);
        }
        connect(mImpl, &PasswordRequesterInterface::done, this, &PasswordRequesterInterface::done);
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

#include "moc_passwordrequester.cpp"
