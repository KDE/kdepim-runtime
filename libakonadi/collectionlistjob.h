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

#ifndef AKONADI_COLLECTIONLIST_JOB
#define AKONADI_COLLECTIONLIST_JOB

#include "libakonadi_export.h"
#include <libakonadi/collection.h>
#include <libakonadi/job.h>

namespace Akonadi {

class CollectionListJobPrivate;

/**
  This class can be used to retrieve the complete or partial collection tree
  from the PIM storage service.
*/
class AKONADI_EXPORT CollectionListJob : public Job
{
  Q_OBJECT

  public:
    /**
      List type.
    */
    enum ListType
    {
      Local, ///< Only fetch the base collection.
      Flat, ///< Only list direct sub-collections of the base collection.
      Recursive ///< List all sub-collections.
    };

    /**
      Create a new CollectionListJob.
      @param collection The base collection for the listing. Must be valid.
      @param type the type of listing to perform
      @param parent The parent object.
    */
    explicit CollectionListJob( const Collection &collection, ListType type = Flat, QObject *parent = 0 );

    /**
      Create a new CollectionListJob to retrieve a list of collections.
      @param cols A list of collections to fetch. Must not be empty, content must be valid.
      @param parent The parent object.
    */
    explicit CollectionListJob( const Collection::List &cols, QObject *parent = 0 );

    /**
      Destroys this job.
    */
    virtual ~CollectionListJob();

    /**
      Returns a list of collection objects.
      It's your job to make sure they are deleted.
    */
    Collection::List collections() const;

    /**
      Sets a resource identifier to limit collection listing to one resource.
      @param resource The resource identifier.
    */
    void setResource( const QString &resource );

    /**
      Include also unsubscribed collections.
    */
    void includeUnsubscribed( bool include = true );

  Q_SIGNALS:
    /**
      Emitted when collections are received.
    */
    void collectionsReceived( const Akonadi::Collection::List &collections );

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  protected Q_SLOTS:
    void slotResult( KJob* job );

  private:
    friend class CollectionListJobPrivate;
    CollectionListJobPrivate* const d;
    Q_PRIVATE_SLOT( d, void timeout() )

};

}

#endif
