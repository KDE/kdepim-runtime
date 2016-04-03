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

namespace Akonadi
{
class Item;
class Collection;
}

class AkonadiSlave : public KIO::SlaveBase
{
public:
    AkonadiSlave(const QByteArray &pool_socket, const QByteArray &app_socket);
    virtual ~AkonadiSlave();

    /**
     * Reimplemented from SlaveBase
     */
    void get(const QUrl &url) Q_DECL_OVERRIDE;

    /**
     * Reimplemented from SlaveBase
     */
    void stat(const QUrl &url) Q_DECL_OVERRIDE;

    /**
     * Reimplemented from SlaveBase
     */
    void listDir(const QUrl &url) Q_DECL_OVERRIDE;

    /**
     * Reimplemented from SlaveBase
     */
    void del(const QUrl &url, bool isFile) Q_DECL_OVERRIDE;

private:
    static KIO::UDSEntry entryForItem(const Akonadi::Item &item);
    static KIO::UDSEntry entryForCollection(const Akonadi::Collection &collection);
};

#endif
