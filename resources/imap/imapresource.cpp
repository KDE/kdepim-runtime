/*
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>
    Copyright (C) 2009 Kevin Ottens <ervin@kde.org>

    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#include "imapresource.h"

#include "setupserver.h"
#include "settings.h"
#include "sessionpool.h"
#include "settingspasswordrequester.h"
#include "sessionuiproxy.h"
#include "tracer.h"
#include <QIcon>

#include <KWindowSystem>
#include <KLocalizedString>

ImapResource::ImapResource(const QString &id)
    : ImapResourceBase(id)
{
    m_pool->setPasswordRequester(new SettingsPasswordRequester(this, m_pool));
    m_pool->setSessionUiProxy(SessionUiProxy::Ptr(new SessionUiProxy));
}

ImapResource::~ImapResource()
{
}

QString ImapResource::defaultName() const
{
    return i18n("IMAP Account");
}

QDialog *ImapResource::createConfigureDialog(WId windowId)
{
    SetupServer *dlg = new SetupServer(this, windowId);
    KWindowSystem::setMainWindow(dlg, windowId);
    dlg->setWindowIcon(QIcon::fromTheme(QStringLiteral("network-server")));
    connect(dlg, &SetupServer::finished, this, &ImapResource::onConfigurationDone);
    return dlg;
}

void ImapResource::onConfigurationDone(int result)
{
    SetupServer *dlg = qobject_cast<SetupServer *>(sender());
    if (result) {
        if (dlg->shouldClearCache()) {
            clearCache();
        }
        settings()->save();
    }
    dlg->deleteLater();
}

void ImapResource::cleanup()
{
    settings()->cleanup();
    Akonadi::AgentBase::cleanup();
}
