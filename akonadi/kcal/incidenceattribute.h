/*
    Copyright (c) 2009 Sebastian Sauer <sebsauer@kdab.com>

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

#ifndef INCIDENCEATTRIBUTE_H
#define INCIDENCEATTRIBUTE_H

#include "akonadi-kcal_next_export.h"

#include <akonadi/item.h>
#include <akonadi/attribute.h>

namespace Akonadi {

class AKONADI_KCAL_NEXT_EXPORT IncidenceAttribute : public Akonadi::Attribute
{
  public:
    explicit IncidenceAttribute();
    ~IncidenceAttribute();

    virtual QByteArray type() const;
    virtual Attribute* clone() const;

    virtual QByteArray serialized() const;
    virtual void deserialize( const QByteArray &data );

    /**
     * The status the invitation is in.
     *
     * One of;
     * "new", "accepted", "tentative", "counter", "cancel", "reply", "delegated"
     */
    QString status() const;
    void setStatus( const QString &newstatus ) const;

    /**
     * The referenced item. This is used e.g. in the invitationagent to
     * let users know where the original mail message is.
     */
    Akonadi::Item::Id reference() const;
    void setReference( Akonadi::Item::Id id );
    
  private:
    class Private;
    Private *const d;
};

}

#endif
