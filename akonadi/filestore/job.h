/*  This file is part of the KDE project
    Copyright (C) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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
  class CollectionFetchJob;
  class ItemCreateJob;
  class ItemDeleteJob;
  class ItemFetchJob;
  class ItemModifyJob;
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

        virtual bool visit( CollectionFetchJob *job ) = 0;

        virtual bool visit( ItemCreateJob *job ) = 0;

        virtual bool visit( ItemDeleteJob *job ) = 0;

        virtual bool visit( ItemFetchJob *job ) = 0;

        virtual bool visit( ItemModifyJob *job ) = 0;

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
