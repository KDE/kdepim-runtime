/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef ITEMTEST_H
#define ITEMTEST_H

#include "wrappedobject.h"

#include <akonadi/collection.h>
#include <akonadi/item.h>

class ItemTest : public QObject, protected WrappedObject
{
  Q_OBJECT
  public:
    ItemTest( QObject *parent = 0 );

    void setParentCollection( const Akonadi::Collection &parent );

  public slots:
    QObject* newInstance();

    void setParentCollection( const QString &path );
    QString mimeType() const;
    void setMimeType( const QString &mimeType );
    void setPayloadFromFile( const QString &fileName );

    void create();

  private:
    Akonadi::Collection mParent;
    Akonadi::Item mItem;
};

Q_DECLARE_METATYPE( ItemTest* )

#endif
