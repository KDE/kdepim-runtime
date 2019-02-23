/*
   Copyright (C) 2019 Daniel Vrátil <dvratil@kde.org>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "kgapiversionattribute.h"

KGAPIVersionAttribute::KGAPIVersionAttribute(uint32_t version)
    : mVersion(version)
{}

QByteArray KGAPIVersionAttribute::type() const
{
    return "KGAPIVersionAttribute";
}

Akonadi::Attribute *KGAPIVersionAttribute::clone() const
{
    return new KGAPIVersionAttribute(mVersion);
}

void KGAPIVersionAttribute::deserialize(const QByteArray &data)
{
    mVersion = data.toUInt();
}

QByteArray KGAPIVersionAttribute::serialized() const
{
    return QByteArray::number(mVersion);
}

