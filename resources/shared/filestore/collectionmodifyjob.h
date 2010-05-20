/*  This file is part of the KDE project
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

#ifndef AKONADI_FILESTORE_COLLECTIONMODIFYJOB_H
#define AKONADI_FILESTORE_COLLECTIONMODIFYJOB_H

#include "job.h"

namespace Akonadi
{
  class Collection;

namespace FileStore
{
  class AbstractJobSession;

/**
 */
class AKONADI_FILESTORE_EXPORT CollectionModifyJob : public Job
{
  friend class AbstractJobSession;

  Q_OBJECT

  public:
    explicit CollectionModifyJob( const Collection &collection, AbstractJobSession *session = 0 );

    virtual ~CollectionModifyJob();

    Collection collection() const;

    virtual bool accept( Visitor *visitor );

  private:
    void handleCollectionModified( const Collection &collection );

  private:
    class Private;
    Private *const d;
};

}
}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
