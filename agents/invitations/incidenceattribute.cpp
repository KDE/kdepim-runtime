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

#include "incidenceattribute.h"

#include <QString>
#include <QTextStream>

using namespace Akonadi;
IncidenceAttribute::IncidenceAttribute()
    : Attribute()
{
}

IncidenceAttribute::~IncidenceAttribute()
{
}

QByteArray IncidenceAttribute::type() const
{
    static const QByteArray sType("incidence");
    return sType;
}

Attribute *IncidenceAttribute::clone() const
{
    IncidenceAttribute *other = new IncidenceAttribute;
    return other;
}

QByteArray IncidenceAttribute::serialized() const
{
    QString data;
    QTextStream out(&data);
    out << mStatus;
    out << mReferenceId;
    return data.toUtf8();
}

void IncidenceAttribute::deserialize(const QByteArray &data)
{
    QString s(QString::fromUtf8(data));
    QTextStream in(&s);
    in >> mStatus;
    in >> mReferenceId;
}

QString IncidenceAttribute::status() const
{
    return mStatus;
}

void IncidenceAttribute::setStatus(const QString &newstatus)
{
    mStatus = newstatus;
}

Akonadi::Item::Id IncidenceAttribute::reference() const
{
    return mReferenceId;
}

void IncidenceAttribute::setReference(Akonadi::Item::Id id)
{
    mReferenceId = id;
}
