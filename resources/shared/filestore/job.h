/*  This file is part of the KDE project
    Copyright (C) 2009 Kevin Krammer <kevin.krammer@gmx.at>
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef AKONADI_FILESTORE_JOB_H
#define AKONADI_FILESTORE_JOB_H

#include "akonadi-filestore_export.h"

#include <KJob>

namespace Akonadi
{

namespace FileStore
{
  class AbstractJobSession;
  class CollectionCreateJob;
  class CollectionDeleteJob;
  class CollectionFetchJob;
  class CollectionModifyJob;
  class CollectionMoveJob;
  class ItemCreateJob;
  class ItemDeleteJob;
  class ItemFetchJob;
  class ItemModifyJob;
  class ItemMoveJob;
  class StoreCompactJob;

/**
 */
class AKONADI_FILESTORE_EXPORT Job : public KJob
{
  friend class AbstractJobSession;

  Q_OBJECT

  public:
    class Visitor
    {
      public:
        virtual ~Visitor() {}

        virtual bool visit( Job *job ) = 0;

        virtual bool visit( CollectionCreateJob *job ) = 0;

        virtual bool visit( CollectionDeleteJob *job ) = 0;

        virtual bool visit( CollectionFetchJob *job ) = 0;

        virtual bool visit( CollectionModifyJob *job ) = 0;

        virtual bool visit( CollectionMoveJob *job ) = 0;

        virtual bool visit( ItemCreateJob *job ) = 0;

        virtual bool visit( ItemDeleteJob *job ) = 0;

        virtual bool visit( ItemFetchJob *job ) = 0;

        virtual bool visit( ItemModifyJob *job ) = 0;

        virtual bool visit( ItemMoveJob *job ) = 0;

        virtual bool visit( StoreCompactJob *job ) = 0;
    };

    enum ErrorCodes
    {
      InvalidStoreState = KJob::UserDefinedError + 1,
      InvalidJobContext
    };

    explicit Job( AbstractJobSession *session = 0 );

    virtual ~Job();

    virtual void start();

    virtual bool accept( Visitor *visitor );

  private:
    class Private;
    Private *d;
};

}
}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
