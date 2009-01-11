/*
    Copyright 2008 Ingo Klöcker <kloecker@kde.org>

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

#ifndef MAILDISPATCHERATTRIBUTE_H
#define MAILDISPATCHERATTRIBUTE_H

#include <akonadi/attribute.h>
#include <akonadi/entity.h>

#include <QDateTime>

/**
 * This agent dispatches mail put into the outbox collection.
 */
class MailDispatcherAttribute : public Akonadi::Attribute
{
  public:
    MailDispatcherAttribute();
    virtual ~MailDispatcherAttribute();

  private:
    // implemented for internal usage
    MailDispatcherAttribute( const MailDispatcherAttribute & );

    // not implemented
    MailDispatcherAttribute & operator=( const MailDispatcherAttribute & );

  private:
    virtual MailDispatcherAttribute * clone() const;

    virtual QByteArray type() const;

    virtual QByteArray serialized() const;
    virtual void deserialize( const QByteArray &data );

    int transport() const;
    void setTransport( int id );

    Akonadi::Entity::Id sentMailCollection() const;
    void setSentMailCollection( Akonadi::Entity::Id id );

    QDateTime dueDate() const;
    void setDueDate( const QDateTime &date );


  private:
    class Private;
    Private * const d;
};

#endif // MAILDISPATCHERATTRIBUTE_H
