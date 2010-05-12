/*  This file is part of the KDE project
    Copyright (C) 2009,2010 Kevin Krammer <kevin.krammer@gmx.at>

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

#ifndef AKONADI_FILESTORE_COLLECTIONFETCHJOB_H
#define AKONADI_FILESTORE_COLLECTIONFETCHJOB_H

#include "job.h"

#include <akonadi/collection.h>

namespace Akonadi
{
  class CollectionFetchScope;

namespace FileStore
{
  class AbstractJobSession;

/**
 */
class AKONADI_FILESTORE_EXPORT CollectionFetchJob : public Job
{
  friend class AbstractJobSession;

  Q_OBJECT

  public:
    enum Type
    {
      Base,
      FirstLevel,
      Recursive
    };

    explicit CollectionFetchJob( const Collection &collection, Type type = FirstLevel, AbstractJobSession *session = 0 );

    virtual ~CollectionFetchJob();

    Type type() const;

    Collection collection() const;

    void setFetchScope( const CollectionFetchScope &fetchScope );

    CollectionFetchScope &fetchScope();

    Collection::List collections() const;

    virtual bool accept( Visitor *visitor );

  Q_SIGNALS:
    void collectionsReceived( const Akonadi::Collection::List &items );

  private:
    void handleCollectionsReceived( const Akonadi::Collection::List &collections );

  private:
    class Private;
    Private *const d;
};

}
}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
