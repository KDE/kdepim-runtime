/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#ifndef SENDJOB_H
#define SENDJOB_H

#include <KCompositeJob>

#include <Akonadi/Item>


/**
  This class takes a prevalidated Item with all the required attributes,
  and sends it using MailTransport.
*/

/*
  Note: this job needs to have subjobs of both Akonadi::Job types and other
  (TransportJob) types.  We cannot subclass from Akonadi::Job because it
  assumes all its subjobs must be Akonadi::Jobs.  (see cast in job.cpp
  Job::startNext().
*/
class SendJob : public KCompositeJob
{
  Q_OBJECT

  public:
    explicit SendJob( const Akonadi::Item &item, QObject *parent = 0 );
    virtual ~SendJob();

    /**
      Sends the item.
    */
    virtual void start();

  protected Q_SLOTS:
    /**
      Called when a subjob finishes.
      (reimplemented from KCompositeJob)
    */
    virtual void slotResult( KJob* );

  private:
    class Private;
    //friend class Private;

    Private *const d;

    Q_PRIVATE_SLOT( d, void doNextStep() )

};


#endif
