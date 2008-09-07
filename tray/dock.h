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


#ifndef DOCK_H
#define DOCK_H

#include <KSystemTrayIcon>
#include <QWidget>

#include <akonadi/agentinstance.h>

class QLabel;
class QAction;

class Tray : public QWidget
{
public:
    Tray();

protected:
    void setVisible( bool );
};

class Dock : public KSystemTrayIcon
{
    Q_OBJECT
    Q_CLASSINFO( "D-Bus Interface", "org.kde.akonaditray" )

public:
    /**
     * Contructor
     * @param parent Parent Widget
     */
    Dock( QWidget *parent );

    /**
     * Destructor
     */
    ~Dock();

public Q_SLOTS:
    void infoMessage( const QString&, const QString& = QString() );
    void errorMessage( const QString&, const QString& = QString() );
    qlonglong getWinId();

private Q_SLOTS:
    void slotInstanceWarning( const Akonadi::AgentInstance&, const QString& );
    void slotInstanceError( const Akonadi::AgentInstance&, const QString& );
    void slotServerStarted();
    void slotServerStopped();
    void slotActivated();
    void slotStopAkonadi();
    void slotStartAkonadi();
    void slotStartBackup();
    void slotStartRestore();
    void slotQuit();

private:
    void updateMenu( bool );
    QAction *m_title;
    QAction *m_stopAction;
    QAction *m_startAction;
    QAction *m_backupAction;
    QAction *m_restoreAction;
};

#endif // DOCK_H

