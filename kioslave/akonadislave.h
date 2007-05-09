/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_SLAVE_H
#define AKONADI_SLAVE_H

#include <kio/slavebase.h>

class AkonadiSlave : public KIO::SlaveBase
{
  public:
    AkonadiSlave( const QByteArray &pool_socket, const QByteArray &app_socket );
    virtual ~AkonadiSlave();

    /**
     * Reimplemented from SlaveBase
     */
    virtual void get( const KUrl &url );

    /**
     * Reimplemented from SlaveBase
     */
    virtual void stat( const KUrl &url );

    /**
     * Reimplemented from SlaveBase
     */
    virtual void listDir( const KUrl &url );

    /**
     * Reimplemented from SlaveBase
     */
    virtual void del( const KUrl &url, bool isFile );
};

#endif
