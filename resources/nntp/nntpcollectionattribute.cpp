/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "nntpcollectionattribute.h"

#include <QDataStream>

NntpCollectionAttribute::NntpCollectionAttribute()
  : CollectionAttribute(),
  mLastArticleId( 0 )
{
}

QByteArray NntpCollectionAttribute::type() const
{
  return "NNTP";
}

NntpCollectionAttribute * NntpCollectionAttribute::clone() const
{
  NntpCollectionAttribute *attr = new NntpCollectionAttribute();
  attr->mLastArticleId = mLastArticleId;
  return attr;
}

QByteArray NntpCollectionAttribute::toByteArray() const
{
  QByteArray data = QByteArray::number( mLastArticleId );
  return data;
}

void NntpCollectionAttribute::setData(const QByteArray & data)
{
  mLastArticleId = data.toInt();
}

int NntpCollectionAttribute::lastArticle() const
{
  return mLastArticleId;
}

void NntpCollectionAttribute::setLastArticle(int last)
{
  mLastArticleId = last;
}
