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

#include <KJob>

#include <Akonadi/Item>


/**
  This class takes a prevalidated Item with all the required attributes,
  sends it using MailTransport, and then stores the result of the sending
  operation in the item.
*/
class SendJob : public KJob
{
  Q_OBJECT

  public:
    explicit SendJob( const Akonadi::Item &item, QObject *parent = 0 );
    virtual ~SendJob();

    /**
      Sends the item.
    */
    virtual void start();

    /**
      If this function is called before the job is started, the SendJob will
      just mark the item as aborted, instead of sending it.
      Do not call this function more than once.
    */
    void setMarkAborted();

    /**
      Aborts sending the item.
      This will give the item an ErrorAttribute of "aborted".
      (No need to call setMarkAborted() if you call abort().)
    */
    void abort();

  private:
    class Private;
    //friend class Private;

    Private *const d;

    Q_PRIVATE_SLOT( d, void doTransport() )
    Q_PRIVATE_SLOT( d, void transportResult( KJob *job ) )
    Q_PRIVATE_SLOT( d, void moveResult( KJob *job ) )
    Q_PRIVATE_SLOT( d, void doEmitResult( KJob *job ) )

};


#endif
