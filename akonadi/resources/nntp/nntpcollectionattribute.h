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

#ifndef AKONADI_NNTPCOLLECTIONATTRIBUTE_H
#define AKONADI_NNTPCOLLECTIONATTRIBUTE_H

#include <akonadi/attribute.h>

/**
  Collection attribute to store information needed for incremental
  collection updates.
*/
class NntpCollectionAttribute : public Akonadi::Attribute
{
  public:
    NntpCollectionAttribute();
    QByteArray type() const;
    NntpCollectionAttribute* clone() const;
    QByteArray serialized() const;
    void deserialize( const QByteArray &data );

    /**
      Returns the serial number of the last fetched article.
    */
    int lastArticle() const;

    /**
      Sets the serial number of the last fetched serial number.
      @param last Serial number of the last fetched article.
    */
    void setLastArticle( int last );

  private:
    qint32 mLastArticleId;
};

#endif
