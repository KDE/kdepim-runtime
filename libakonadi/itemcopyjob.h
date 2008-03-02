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

#ifndef AKONADI_ITEMCOPYJOB_H
#define AKONADI_ITEMCOPYJOB_H

#include <libakonadi/collection.h>
#include <libakonadi/item.h>
#include <libakonadi/job.h>

namespace Akonadi {

/**
  Copies a set of items into the specified target collection.
*/
class AKONADI_EXPORT ItemCopyJob : public Job
{
  Q_OBJECT

  public:
    /**
      Create a new copy job to copy the given item into @p target.
      @param item The item to copy.
      @param target The target collection.
      @param parent The parent object.
    */
    ItemCopyJob( const Item &item, const Collection &target, QObject *parent = 0 );

    /**
      Create a new copy job to copy the given items into @p target.
      @param items A list of items to copy.
      @param target The target collection.
      @param parent The parent object.
    */
    ItemCopyJob( const Item::List &items, const Collection &target, QObject *parent = 0 );

    /**
      Destructor.
    */
    ~ItemCopyJob();

  protected:
    void doStart();

  private:
    class Private;
    Private* const d;
};

}

#endif
