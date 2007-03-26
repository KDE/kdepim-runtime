/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_MESSAGEFETCHJOB_H
#define AKONADI_MESSAGEFETCHJOB_H

#include "itemfetchjob.h"

#include <libakonadi/itemfetchjob.h>
#include <libakonadi/message.h>

namespace Akonadi {

class MessageFetchJobPrivate;

/**
  Fetches message data from the backend.
*/
class AKONADI_EXPORT MessageFetchJob : public ItemFetchJob
{
  Q_OBJECT
  public:
    /**
      Create a new message fetch job.
      @param parent The parent object.
    */
    explicit MessageFetchJob( QObject *parent = 0 );

    /**
      Create a new message fetch job to retrieve envelopes of all
      messages in the given collection.
      @param collection The collection.
      @param parent The parent object.
    */
    explicit MessageFetchJob( const Collection &collection, QObject *parent = 0 );

    /**
      Creates a new message fetch job to retrieve the complete message
      with the given uid.
      @param ref The unique message id.
      @param parent The parent object.
    */
    explicit MessageFetchJob( const DataReference &ref, QObject *parent = 0 );

    /**
      Destroys this job.
    */
    virtual ~MessageFetchJob();

    /**
      Returns the fetched message objects. Invalid before the result(KJob*)
      signal has been emitted or if an error occurred.
    */
    Message::List messages() const;

    virtual Item::List items() const;

  protected:
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    MessageFetchJobPrivate* const d;

};

}

#endif
