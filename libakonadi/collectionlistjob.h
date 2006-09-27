/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#ifndef AKONADI_COLLECTION_JOB
#define AKONADI_COLLECTION_JOB

#include <libakonadi/collection.h>
#include <libakonadi/job.h>
#include <kdepim_export.h>

namespace Akonadi {

class Collection;
class CollectionListJobPrivate;

/**
  This class can be used to retrieve the complete or partial collection tree
  of the PIM storage service.

  It returns a QHash of references to Akonadi::Collection objects.

  @todo Add partial collection retrieval (eg. only collections containing contacts).
*/
class AKONADI_EXPORT CollectionListJob : public Job
{
  Q_OBJECT

  public:
    /**
      Create a new CollectionListJob.
      @param prefix The folder that should be listed
      @param recursive Do a recursive collection listing.
      @param parent The parent object.
    */
    CollectionListJob( const QString &prefix, bool recursive = false, QObject *parent = 0 );

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

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    CollectionListJobPrivate *d;

};

}

#endif
