/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTIONSELECTJOB_H
#define AKONADI_COLLECTIONSELECTJOB_H

#include <libakonadi/collection.h>
#include <libakonadi/job.h>

namespace Akonadi {

class CollectionSelectJobPrivate;

/**
  Selects a specific collection. See RFC 3501 for select semantics.
*/
class AKONADI_EXPORT CollectionSelectJob : public Job
{
  Q_OBJECT
  public:
    /**
      Creates a new collection select job.
      @param collection The collection to select.
      @param parent The parent object.
    */
    explicit CollectionSelectJob( const Collection &collection, QObject *parent = 0 );

    /**
      Destroys this job.
    */
    virtual ~CollectionSelectJob();

    /**
      Enable fetching of collection status details.
    */
    void setRetrieveStatus( bool status );

    /**
      Returns the unseen count of the selected folder, -1 if not available.
    */
    int unseen() const;

  protected:
    void doStart();
    void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    CollectionSelectJobPrivate* const d;
};

}

#endif
