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
#include "backup.h"
#include "trayadaptor.h"

#include <QDBusInterface>
#include <QDBusConnectionInterface>
#include <QMouseEvent>
#include <QLabel>
#include <QToolButton>
#include <QWidgetAction>

#include <KComponentData>
#include <KDebug>
#include <KFileDialog>
#include <KIcon>
#include <KIconLoader>
#include <KLocale>
#include <KMenu>
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
        : KSystemTrayIcon( KIcon( "dummy" ), parent )
{
    KMenu *menu = new KMenu();
    m_title = menu->addTitle( i18n( "Akonadi" ) );

    new TrayAdaptor( this );
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject( "/Actions", this );

    m_stopAction = menu->addAction( i18n( "&Stop Akonadi" ), this, SLOT( slotStopAkonadi() ) );
    m_startAction = menu->addAction( i18n( "S&tart Akonadi" ), this, SLOT( slotStartAkonadi() ) );
    m_backupAction = menu->addAction( i18n( "Make &backup" ), this, SLOT( slotStartBackup() ) );
    menu->addSeparator();
    menu->addAction( KIcon( "application-exit" ), i18n( "Quit" ), this, SLOT( slotQuit() ),
                     KStandardShortcut::shortcut( KStandardShortcut::Quit ).primary() );

    setContextMenu( menu );
    connect( menu, SIGNAL( aboutToShow() ), SLOT( slotActivated() ) );
    show();

    connect( QDBusConnection::sessionBus().interface(),
             SIGNAL( serviceOwnerChanged( const QString&, const QString&, const QString& ) ),
             SLOT( slotServiceChanged( const QString&, const QString&, const QString& ) ) );
}

Dock::~Dock()
{
}


void Dock::slotServiceChanged( const QString& service, const QString& oldOwner, const QString& newOwner )
{
    if ( service != "org.kde.Akonadi.Control" )
        return;

    if ( oldOwner.isEmpty() ) {
        updateMenu( true );
        KPassivePopup::message( i18n( "Akonadi available" ),
                                i18n( "The Akonadi server has been started and can be used now." ), this );
    } else if ( newOwner.isEmpty() ) {
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

void Dock::slotStartBackup()
{
    bool registered = QDBusConnection::sessionBus().interface()->isServiceRegistered( "org.kde.Akonadi.Control" );
    Q_ASSERT( registered );

    Backup* backup = new Backup( 0 );
    connect( backup, SIGNAL( completed( bool ) ), SLOT( slotBackupComplete( bool ) ) );
    bool possible = backup->possible();
    if ( !possible ) {
        KMessageBox::error( 0, i18n( "The backup can not be made. Either the mysqldump application "
                                     "is not installed, or the bzip2 application is not found. Please install those and "
                                     "make sure they can be found in the current path" ) );
        return;
    }

    const QString filename = KFileDialog::getSaveFileName( KUrl( "~/akonadibackup.tgz" ) );
    if ( !filename.isEmpty() )
        backup->create( filename );
}

void Dock::slotBackupComplete( bool success )
{
    kDebug() << success;
    if ( success )
        KMessageBox::information( 0, "backup was a success" );
    else
        KMessageBox::information( 0, "backup failed" );
}

void Dock::updateMenu( bool registered )
{
    /* kdelibs... */
    QToolButton *button = static_cast<QToolButton*>(( static_cast<QWidgetAction*>( m_title ) )->defaultWidget() );
    QAction* action = button->defaultAction();
    action->setText( registered ? i18n( "Akonadi is running" ) :  i18n( "Akonadi is not running" ) );
    button->setDefaultAction( action );

    m_stopAction->setVisible( registered );
    m_backupAction->setEnabled( registered );
    m_startAction->setVisible( !registered );
}

void Dock::infoMessage( const QString &message )
{
    KPassivePopup::message( i18n( "Akonadi message" ), message, this );
}

void Dock::errorMessage( const QString &message )
{
    KMessageBox::error( 0, message, i18n( "Akonadi error" ) );
}

qlonglong Dock::getWinId()
{
    return ( qlonglong )parentWidget()->winId();
}

void Dock::slotQuit()
{
    exit( 0 );
}

#include "dock.moc"
