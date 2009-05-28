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

#ifndef OUTBOXINTERFACE_MESSAGEQUEUEJOB_H
#define OUTBOXINTERFACE_MESSAGEQUEUEJOB_H

#include <outboxinterface/outboxinterface_export.h>

#include "dispatchmodeattribute.h"

#include <QtCore/QDateTime>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <KDE/KCompositeJob>

#include <Akonadi/Collection>

#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>


namespace OutboxInterface {


/**
  This is the preferred interface for sending email from applications.

  Example:
  @code

  TODO

*/
class OUTBOXINTERFACE_EXPORT MessageQueueJob : public KCompositeJob
{
  Q_OBJECT

  public:
    explicit MessageQueueJob( QObject *parent = 0 );
    virtual ~MessageQueueJob();

    //TODO there is a lot of duplication between these and the attributes.
    // Any better way to handle this?

    //TODO document all of these
    KMime::Message::Ptr message() const;
    int transportId() const;
    DispatchModeAttribute::DispatchMode dispatchMode() const;
    QDateTime sendDueDate() const;
    Akonadi::Collection::Id sentMailCollection() const;
    QString from() const;
    QStringList to() const;
    QStringList cc() const;
    QStringList bcc() const;

    void setMessage( KMime::Message::Ptr message );
    void setTransportId( int id );
    void setDispatchMode( DispatchModeAttribute::DispatchMode mode );
    void setDueDate( const QDateTime &date );
    void setSentMailCollection( Akonadi::Collection::Id id );
    void setFrom( const QString &from );
    void setTo( const QStringList &to );
    void setCc( const QStringList &cc );
    void setBcc( const QStringList &bcc );

    /**
      Reads From, To, Cc, Bcc from the MIME message.
    */
    void readAddressesFromMime();

    /**
      Creates the item and places it in the outbox.
      It is now queued for sending by the mail dispatcher agent.

      (reimplemented from KJob)
    */
    virtual void start();

  protected Q_SLOTS:
    /**
      Called when the subjob finishes.

      (reimplemented from KCompositeJob)
    */
    virtual void slotResult( KJob * );

  private:
    class Private;
    //friend class Private;

    Private *const d;

    Q_PRIVATE_SLOT( d, void doStart() )

};


}


#endif
