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

#include <qsystemtrayicon.h>

class Dock : public QSystemTrayIcon
{
    Q_OBJECT

public:
    /**
     * Contructor
     * @param parent Parent Widget
     * @param name Name
     */
    Dock();

    /**
     * Destructor
     */
    ~Dock();

protected:
    void mousePressEvent( QMouseEvent *e );

private slots:
    void slotConfigureNotifications();
    void slotStopAkonadi();
    void slotStartAkonadi();
    void slotQuit();
};

#endif // DOCK_H

