/*
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

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

#ifndef OUTBOXINTERFACE_DISPATCHMODEATTRIBUTE_H
#define OUTBOXINTERFACE_DISPATCHMODEATTRIBUTE_H

#include <outboxinterface/outboxinterface_export.h>

#include <QtCore/QDateTime>

#include <Akonadi/Attribute>


namespace OutboxInterface
{


/**
 * Attribute determining how and when a message from the outbox is dispatched.
 *
 * TODO: name: SendPolicy? SendingPolicy?
 */
class OUTBOXINTERFACE_EXPORT DispatchModeAttribute : public Akonadi::Attribute
{
  public:
    enum DispatchMode
    {
      Immediately,
      AfterDueDate,
      Never
    };

    explicit DispatchModeAttribute( DispatchMode mode = Immediately, const QDateTime &date = QDateTime() );
    virtual ~DispatchModeAttribute();

    virtual DispatchModeAttribute* clone() const;
    virtual QByteArray type() const;
    virtual QByteArray serialized() const;
    virtual void deserialize( const QByteArray &data );

    DispatchMode dispatchMode() const;
    void setDispatchMode( DispatchMode mode );
    QDateTime dueDate() const;
    void setDueDate( const QDateTime &date );

  private:
    DispatchMode mMode;
    QDateTime mDueDate;

};


}


#endif
