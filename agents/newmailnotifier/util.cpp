/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "util.h"

#include <KNotification>
#include <KLocalizedString>
#include <KGlobal>
#include <KIcon>
#include <KIconLoader>
#include <KToolInvocation>

#include <Akonadi/AgentInstance>
#include <Akonadi/AgentType>
#include <KMime/Message>

#include <QDBusInterface>
#include <QDBusConnection>
#include <QDBusConnectionInterface>


void Util::testJovieService()
{
    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(QLatin1String("org.kde.kttsd"))) {
        QString error;
        if (KToolInvocation::startServiceByDesktopName(QLatin1String("kttsd"), QStringList(), &error)) {
            KNotification::event( QLatin1String("text-to-speak-not-found"),
                                  i18n("Starting Jovie Text-to-Speech Service Failed %1", error),
                                  Util::defaultPixmap(),
                                  0,
                                  KNotification::CloseOnTimeout,
                                  KGlobal::mainComponent());
        }
    }
}

void Util::showNotification(const QPixmap &pixmap, const QString &message)
{
    KNotification::event( QLatin1String("new-email"),
                          message,
                          pixmap,
                          0,
                          KNotification::CloseOnTimeout,
                          KGlobal::mainComponent());
}

QPixmap Util::defaultPixmap()
{
    const QPixmap pixmap = KIcon( QLatin1String("kmail") ).pixmap( KIconLoader::SizeMedium, KIconLoader::SizeMedium );
    return pixmap;
}

bool Util::excludeAgentType(const Akonadi::AgentInstance &instance)
{
    if ( instance.type().mimeTypes().contains( KMime::Message::mimeType() ) ) {
        const QStringList capabilities( instance.type().capabilities() );
        if ( capabilities.contains( QLatin1String("Resource") ) &&
             !capabilities.contains( QLatin1String("Virtual") ) &&
             !capabilities.contains( QLatin1String("MailTransport") ) ) {
            return false;
        } else {
            return true;
        }
    }
    return true;
}
