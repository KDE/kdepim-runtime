/* This file is part of the KDE project
   Copyright (C) 2008-2009 Omat Holding B.V. <info@omat.nl>

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
#include "backupassistant.h"
#include "restoreassistant.h"
#include "akonaditrayadaptor.h"

#include <QDBusInterface>
#include <QDBusConnectionInterface>
#include <QMouseEvent>
#include <QToolButton>
#include <QWidgetAction>

#include <KComponentData>
#include <KDebug>
#include <KIcon>
#include <KIconLoader>
#include <KLocale>
#include <KMenu>
#include <KMessageBox>
#include <KNotification>
#include <KStandardShortcut>

#include <akonadi/control.h>
#include <akonadi/agentinstance.h>
#include <akonadi/agentmanager.h>
#include <akonadi/servermanager.h>

using namespace Akonadi;

Tray::Tray() : QWidget()
{
    hide();
    new Dock( this );
}

void Tray::setVisible( bool )
{
    // We don't want this Widget to get visible because of a click on the tray icon.
    QWidget::setVisible( false );
}

Dock::Dock( QWidget *parent )
        : KStatusNotifierItem( 0 ), m_explicitStart( false )
{
    m_parentWidget = parent;

    setIconByName("akonadi");
    setCategory(SystemServices);
    setStatus(Passive);
    KMenu *menu = new KMenu();
    m_title = menu->addTitle( i18n( "Akonadi" ) );

    new AkonaditrayAdaptor( this );
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject( "/Actions", this );

    m_stopAction = menu->addAction( i18n( "&Stop Akonadi" ), this, SLOT(slotStopAkonadi()) );
    m_startAction = menu->addAction( i18n( "S&tart Akonadi" ), this, SLOT(slotStartAkonadi()) );
    m_backupAction = menu->addAction( i18n( "Make &Backup..." ), this, SLOT(slotStartBackup()) );
    m_restoreAction = menu->addAction( i18n( "&Restore Backup..." ), this, SLOT(slotStartRestore()) );
    menu->addAction( KIcon( "configure" ), i18n( "&Configure..." ), this, SLOT(slotConfigure()) );

    setContextMenu( menu );
    connect( menu, SIGNAL(aboutToShow()), SLOT(slotActivated()) );

    connect( ServerManager::self(), SIGNAL(started()), SLOT(slotServerStarted()) );
    connect( ServerManager::self(), SIGNAL(stopped()), SLOT(slotServerStopped()) );

    AgentManager *manager = AgentManager::self();
    connect( manager,
             SIGNAL(instanceWarning(Akonadi::AgentInstance,QString)),
             SLOT(slotInstanceWarning(Akonadi::AgentInstance,QString)) );
    connect( manager,
             SIGNAL(instanceError(Akonadi::AgentInstance,QString)),
             SLOT(slotInstanceError(Akonadi::AgentInstance,QString)) );
}

Dock::~Dock()
{
}

void Dock::slotServerStarted()
{
    updateMenu( true );
    if ( m_explicitStart ) {
      showMessage( i18n( "Akonadi available" ),
                   i18n( "The Akonadi server has been started and can be used now." ), "akonadi" );
    m_explicitStart = false;
    }
}

void Dock::slotServerStopped()
{
    updateMenu( false );
    showMessage( i18n( "Akonadi not available" ),
                 i18n( "The Akonadi server has been stopped, Akonadi related applications can no longer be used." ), "akonadi" );
}

void Dock::slotStopAkonadi()
{
    Akonadi::Control::stop( m_parentWidget );
}

void Dock::slotStartAkonadi()
{
    m_explicitStart = true;
    Akonadi::Control::start( m_parentWidget );
}

void Dock::slotActivated()
{
    updateMenu( ServerManager::isRunning() );
}

void Dock::slotStartBackup()
{
    bool registered = ServerManager::isRunning();
    Q_ASSERT( registered );
    Q_UNUSED( registered );

    QPointer<BackupAssistant> backup = new BackupAssistant( m_parentWidget );
    backup->exec();
    delete backup;
}

void Dock::slotStartRestore()
{
    bool registered = ServerManager::isRunning();
    Q_ASSERT( registered );
    Q_UNUSED( registered );

    QPointer<RestoreAssistant> restore = new RestoreAssistant( m_parentWidget );
    restore->exec();
    delete restore;
}

void Dock::updateMenu( bool registered )
{
    if ( registered ) {
          setToolTip( "akonadi", i18n( "Akonadi available" ),
                                 i18n( "The Akonadi server has been started and can be used now." ) );
    } else {
          setToolTip( "akonadi", i18n( "Akonadi not available" ),
                                 i18n( "The Akonadi server has been stopped, Akonadi related applications can no longer be used." ) );
    }

    /* kdelibs... */
    QToolButton *button = static_cast<QToolButton*>(( static_cast<QWidgetAction*>( m_title ) )->defaultWidget() );
    QAction* action = button->defaultAction();
    action->setText( registered ? i18n( "Akonadi is running" ) :  i18n( "Akonadi is not running" ) );
    button->setDefaultAction( action );

    m_stopAction->setVisible( registered );
    m_backupAction->setEnabled( registered );
    m_restoreAction->setEnabled( registered );
    m_startAction->setVisible( !registered );
}

void Dock::slotInstanceWarning( const Akonadi::AgentInstance& agent, const QString& message )
{
    QString msg = message;
    if ( !agent.name().isEmpty() )
      msg = i18nc( "<source>: <error message>", "%1: %2", agent.name(), msg );
    infoMessage( msg, agent.name() );
}

void Dock::infoMessage( const QString &message, const QString &title )
{
    KNotification::event( KNotification::Notification, title.isEmpty() ? i18n( "Akonadi message" ) : title,
                            message, QPixmap(), m_parentWidget  );
}

void Dock::slotInstanceError( const Akonadi::AgentInstance& agent, const QString& message )
{
    QString msg = message;
    if ( !agent.name().isEmpty() )
      msg = i18nc( "<source>: <error message>", "%1: %2", agent.name(), msg );
    errorMessage( msg, agent.name() );
}

void Dock::errorMessage( const QString &message, const QString &title )
{
    KNotification::event( KNotification::Error, title.isEmpty() ? i18n( "Akonadi error" ) : title,
                          message, DesktopIcon( "dialog-warning" ), m_parentWidget );
}

qlonglong Dock::getWinId()
{
    return ( qlonglong )m_parentWidget ->winId();
}

void Dock::slotConfigure()
{
    QProcess *proc = new QProcess( this );
    proc->start( "kcmshell4", QStringList() << "kcm_akonadi" );
}

#include "dock.moc"
