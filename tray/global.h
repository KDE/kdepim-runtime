/* This file is part of the KDE project

   Copyright (C) 2006-2007 Omat Holding B.V. <info@omat.nl>
   Copyright (C) 2007 Frode M. Døving <frode@lnix.net>

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


#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>
#include <QObject>

namespace Tray
{

class Global
{
public:
    Global() : m_parsed( false ) {};

    /**
     * Returns a string usuable to connect to the akonadiserver.
     */
    const QString dboptions();

    /**
     * Returns the database to connect to.
     */
    const QString dbname();

private:
    void init();

    bool m_parsed;
    QString m_dboptions;
    QString m_dbname;
};

}

#endif
