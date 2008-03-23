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
#include "trayadaptor.h"

#include <QMenu>
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
#include <KMessageBox>
#include <KPassivePopup>
#include <KStandardShortcut>
#include <KSystemTrayIcon>

#include <akonadi/control.h>

Tray::Tray() : QWidget()
{
    hide();
    Dock* docker = new Dock( this );
    docker->show();
}

void Tray::setVisible( bool )
{
    // We don't want this Widget to get visible because of a click on the tray icon.
    QWidget::setVisible( false );
}


Dock::Dock( QWidget *parent )
        : KSystemTrayIcon( KIcon("dummy"), parent )
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

    new TrayAdaptor(  this );
        QDBusConnection dbus = QDBusConnection::sessionBus();
                        dbus.registerObject(  "/Actions", this );

    m_stopAction = menu->addAction( i18n( "&Stop Akonadi" ), this, SLOT( slotStopAkonadi() ) );
    m_startAction = menu->addAction( i18n( "&Start Akonadi" ), this, SLOT( slotStartAkonadi() ) );
    menu->addSeparator();
    menu->addAction( KIcon( "application-exit" ), i18n( "Quit" ), this, SLOT( slotQuit() ),
                     KStandardShortcut::shortcut( KStandardShortcut::Quit ).primary() );

    setContextMenu( menu );
    connect( menu, SIGNAL( aboutToShow() ), SLOT( slotActivated() ) );
    show();

    connect(QDBusConnection::sessionBus().interface(), 
        SIGNAL( serviceOwnerChanged( const QString&, const QString&, const QString& ) ),
        SLOT( slotServiceChanged( const QString&, const QString&, const QString& ) ) );
}

Dock::~Dock()
{
}


void Dock::slotServiceChanged( const QString& service, const QString& oldOwner, const QString& newOwner )
{
    if ( service != "org.kde.Akonadi.Control") 
        return;

    if (oldOwner.isEmpty() ) {
	updateMenu( true );
	KPassivePopup::message( i18n( "Akonadi available" ), 
                     i18n( "The Akonadi server has been started and can be used now." ), this );
    } else if (newOwner.isEmpty() ) {
	updateMenu( false );
	KPassivePopup::message( i18n( "Akonadi not available" ), 
                     i18n( "The Akonadi server has been stopped, Akonadi related application can no longer be used." ),this );
    }
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
    bool registered = QDBusConnection::sessionBus().interface()->isServiceRegistered( "org.kde.Akonadi.Control" );
    updateMenu( registered );
}

void Dock::updateMenu( bool registered )
{
    registered ? m_title->setText( i18n( "Akonadi is running" ) ) : m_title->setText( i18n( "Akonadi is not running" ) );
    m_stopAction->setVisible( registered );
    m_startAction->setVisible( !registered );
}

void Dock::infoMessage( const QString &message )
{
    KPassivePopup::message( i18n("Akonadi message"), message, this );
}

void Dock::errorMessage( const QString &message )
{
    KMessageBox::error( 0, message, i18n("Akonadi error") );
}

WId Dock::getWinId()
{
    return parentWidget()->winId();
}

void Dock::slotQuit()
{
    exit( 0 );
}

#include "dock.moc"
