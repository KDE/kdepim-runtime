/*
    Copyright (c) 2015 Grégory Oestreicher <greg@kamago.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "ctagattribute.h"

CTagAttribute::CTagAttribute(const QString& ctag)
    : mCTag(ctag)
{
}

void CTagAttribute::setCTag(const QString& ctag)
{
    mCTag = ctag;
}

QString CTagAttribute::CTag() const
{
    return mCTag;
}

Akonadi::Attribute *CTagAttribute::clone() const
{
    return new CTagAttribute(mCTag);
}

QByteArray CTagAttribute::type() const
{
    static const QByteArray sType("ctag");
    return sType;
}

QByteArray CTagAttribute::serialized() const
{
    return mCTag.toUtf8();
}

void CTagAttribute::deserialize(const QByteArray &data)
{
    mCTag = QString::fromUtf8(data);
}