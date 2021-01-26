/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
    ~AkonadiSlave() override;

    /**
     * Reimplemented from SlaveBase
     */
    void get(const QUrl &url) override;

    /**
     * Reimplemented from SlaveBase
     */
    void stat(const QUrl &url) override;

    /**
     * Reimplemented from SlaveBase
     */
    void listDir(const QUrl &url) override;

    /**
     * Reimplemented from SlaveBase
     */
    void del(const QUrl &url, bool isFile) override;

private:
    static KIO::UDSEntry entryForItem(const Akonadi::Item &item);
    static KIO::UDSEntry entryForCollection(const Akonadi::Collection &collection);
};

#endif
