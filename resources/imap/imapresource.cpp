/*
    SPDX-FileCopyrightText: 2007 Till Adam <adam@kde.org>
    SPDX-FileCopyrightText: 2008 Omat Holding B.V. <info@omat.nl>
    SPDX-FileCopyrightText: 2009 Kevin Ottens <ervin@kde.org>

    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    SPDX-FileContributor: Kevin Ottens <kevin@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "imapresource.h"

#include "sessionpool.h"
#include "sessionuiproxy.h"
#include "settings.h"
#include <config-imap.h>

#ifdef WITH_GMAIL_XOAUTH2
#include "passwordrequester.h"
#else
#include "settingspasswordrequester.h"
#endif

#include <QIcon>

#include <KLocalizedString>
#include <KWindowSystem>

ImapResource::ImapResource(const QString &id)
    : ImapResourceBase(id)
{
#ifdef WITH_GMAIL_XOAUTH2
    m_pool->setPasswordRequester(new PasswordRequester(this, m_pool));
#else
    m_pool->setPasswordRequester(new SettingsPasswordRequester(this, m_pool));
#endif
    m_pool->setSessionUiProxy(SessionUiProxy::Ptr(new SessionUiProxy));
    m_pool->setClientId(clientId());

    settings(); // make sure the D-Bus settings interface is up

    if (settings()->name().isEmpty()) {
        if (name() == identifier()) {
            settings()->setName(defaultName());
        } else {
            settings()->setName(name());
        }
    }
    setActivitiesEnabled(settings()->activitiesEnabled());
    setActivities(settings()->activities());
    setName(settings()->name());

    connect(this, &ImapResource::reloadConfiguration, this, &ImapResource::slotConfigurationChanged);
}

ImapResource::~ImapResource() = default;

QString ImapResource::defaultName() const
{
    return i18n("IMAP Account");
}

QByteArray ImapResource::clientId() const
{
    return QByteArrayLiteral("Kontact IMAP Resource");
}

void ImapResource::slotConfigurationChanged()
{
    const auto oldImapServer = settings()->imapServer();
    const auto oldUserName = settings()->userName();

    settings()->load();

    const auto newImapServer = settings()->imapServer();
    const auto newUserName = settings()->userName();

    if (oldImapServer != newImapServer || oldUserName != newUserName) {
        clearCache();
    }

    setActivitiesEnabled(settings()->activitiesEnabled());
    setActivities(settings()->activities());
    setName(settings()->name());

    reconnect();
}

#include "moc_imapresource.cpp"
