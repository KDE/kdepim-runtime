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

#include <item.h>
#include <attribute.h>

namespace Akonadi {
class IncidenceAttribute : public Akonadi::Attribute
{
public:
    explicit IncidenceAttribute();
    ~IncidenceAttribute();

    QByteArray type() const override;
    Attribute *clone() const override;

    QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

    /**
     * The status the invitation is in.
     *
     * One of;
     * "new", "accepted", "tentative", "counter", "cancel", "reply", "delegated"
     */
    QString status() const;
    void setStatus(const QString &newstatus);

    /**
     * The referenced item. This is used e.g. in the invitationagent to
     * let users know where the original mail message is.
     */
    Akonadi::Item::Id reference() const;
    void setReference(Akonadi::Item::Id id);

private:
    QString mStatus;
    Akonadi::Item::Id mReferenceId;
};
}

#endif
