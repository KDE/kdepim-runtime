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

#ifndef PIM_ITEMDELETEJOB_H
#define PIM_ITEMDELETEJOB_H

#include <libakonadi/job.h>

namespace PIM {

class ItemDeleteJobPrivate;

/**
  Convenience job which permanently deletes an item, ie. sets the \Deleted flag
  and then executes the EXPUNGE command.
*/
class ItemDeleteJob : public Job
{
  Q_OBJECT

  public:
    /**
      Cretes a new ItemDeleteJob.
      @param ref The reference of the item to delete.
      @param parent The parent object.
    */
    ItemDeleteJob( const DataReference &ref, QObject *parent = 0 );

    /**
      Destorys this job.
    */
    ~ItemDeleteJob();

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray& tag, const QByteArray &data );

  private slots:
    void storeDone( PIM::Job* job );
    void expungeDone( PIM::Job* job );

  private:
    ItemDeleteJobPrivate *d;

};

}

#endif
