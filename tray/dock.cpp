/* This file is part of the KDE project
   Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "dock.h"

#include <QMenu>
#include <QSystemTrayIcon>
#include <QDBusInterface>
#include <QDBusConnectionInterface>
#include <QMouseEvent>
#include <QLabel>
#include <QWidgetAction>

#include <KComponentData>
#include <KDebug>
#include <KIcon>
#include <KIconLoader>
#include <KLocale>
#include <KNotifyConfigWidget>
#include <KStandardShortcut>
#include <KSystemTrayIcon>

#include <akonadi/control.h>

Dock::Dock()
        : KSystemTrayIcon( KIcon("dummy"), 0 )
{
    QMenu *menu = new QMenu();

    /* This should be solved in kdelibs ---- */
    QWidgetAction* action = new QWidgetAction(this);
    action->setEnabled(false);
    m_title = new QLabel(0);
    action->setDefaultWidget(m_title);
    m_title->setFrameShape(QFrame::Box);
    m_title->setText(i18n("Akonadi"));
    QFont f = m_title->font();
    f.setBold(true);
    m_title->setFont(f);
    menu->addAction(action);
    /* End */

    menu->addAction( i18n( "&Stop Akonadi" ),
                     this, SLOT( slotStopAkonadi() ) );
    menu->addAction( i18n( "&Start Akonadi" ),
                     this, SLOT( slotStartAkonadi() ) );
    menu->addSeparator();
    menu->addAction( /*SmallIcon( "knotify" ),*/ i18n( "Configure &Notifications..." ),
                     this, SLOT( slotConfigureNotifications() ) );
    menu->addAction( KIcon( "application-exit" ), i18n( "Quit" ), this, SLOT( slotQuit() ),
                     KStandardShortcut::shortcut( KStandardShortcut::Quit ).primary() );

    setContextMenu( menu );
    connect( menu, SIGNAL( aboutToShow() ), SLOT( slotActivated() ) );
    show();
}

Dock::~Dock()
{
}


void Dock::slotConfigureNotifications()
{
    KNotifyConfigWidget::configure( 0 );
}

void Dock::slotStopAkonadi()
{
     QDBusInterface dbus( "org.kde.Akonadi.Control", "/ControlManager", "org.kde.Akonadi.ControlManager" );
     dbus.call( "shutdown" );
}

void Dock::slotStartAkonadi()
{
     Akonadi::Control::start();
}

void Dock::slotActivated()
{
    kDebug();
    bool registered = QDBusConnection::sessionBus().interface()->isServiceRegistered( "org.kde.Akonadi.Control" );
    registered ? m_title->setText( i18n( "Akonadi is running" ) ) 
	        : m_title->setText( i18n( "Akonadi is not running" ) );
}

void Dock::slotQuit()
{
    exit( 0 );
}

#include "dock.moc"
