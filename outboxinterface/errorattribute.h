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

#ifndef OUTBOXINTERFACE_ERRORATTRIBUTE_H
#define OUTBOXINTERFACE_ERRORATTRIBUTE_H

#include <outboxinterface/outboxinterface_export.h>

#include <QString>

#include <Akonadi/Attribute>


namespace OutboxInterface
{


/**
 * Attribute storing the status of a message in the outbox.
 */
class OUTBOXINTERFACE_EXPORT ErrorAttribute : public Akonadi::Attribute
{
  public:
    ErrorAttribute( const QString &msg = QString() );
    virtual ~ErrorAttribute();

    virtual ErrorAttribute* clone() const;
    virtual QByteArray type() const;
    virtual QByteArray serialized() const;
    virtual void deserialize( const QByteArray &data );

    /**
      Textual explanation of error.
    */
    QString message() const;
    void setMessage( const QString &msg );

  private:
    QString mMessage;

};


}


#endif
