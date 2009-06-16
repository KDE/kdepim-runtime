/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef KABCMIGRATOR_H
#define KABCMIGRATOR_H

#include "kresmigrator.h"

#include <kabc/resource.h>

#include <QHash>

class KJob;

/**
 * Migrate KABC resources to Akonadi
 */
class KABCMigrator : public KResMigrator<KABC::Resource>
{
  Q_OBJECT
  public:
    KABCMigrator();

    bool migrateResource( KABC::Resource *res );

  private slots:
    void fileResourceCreated( KJob* job );

  private:
    void migrateFileResource( KABC::Resource *res );
};

#endif
