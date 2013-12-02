/*
  Copyright (c) 2013 Christian Mollekopf <mollekopf@kolabsys.com>

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
#ifndef HANDLERMANAGER_H
#define HANDLERMANAGER_H

#include <KJob>
#include <Akonadi/Monitor>
#include "kolabhandler.h"

class HandlerManager
{
public:
    bool isMonitored(Akonadi::Collection::Id) const;
    KolabHandler::Ptr getHandler(Akonadi::Collection::Id);

    /**
     * Creates a new KolabHandler for @p imapCollection given it actually is
     * a Kolab folder.
     *
     * @return @c true if @p imapCollection is a Kolab folder, @c false otherwise.
     */
    bool registerHandlerForCollection(const Akonadi::Collection &imapCollection, Kolab::Version version);

    QString imapResourceForCollection(Akonadi::Collection::Id id);

    void removeFolder(Akonadi::Collection::Id imapCollection);

    QList<Akonadi::Collection::Id> monitoredCollections() const;

    void clear();

    static bool isKolabFolder( const Akonadi::Collection &collection );
    static bool isHandledKolabFolder( const Akonadi::Collection &collection );
    static Kolab::FolderType getFolderType( const Akonadi::Collection &collection );
private:
    QMap<Akonadi::Collection::Id, KolabHandler::Ptr> mMonitoredCollections;
    QMap<Akonadi::Collection::Id, QString> mResourceIdentifiers;

};

#endif