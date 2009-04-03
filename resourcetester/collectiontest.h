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

#ifndef COLLECTIONTEST_H
#define COLLECTIONTEST_H

#include <QObject>

#include <akonadi/collection.h>

class CollectionTest : public QObject
{
  Q_OBJECT
  public:
    CollectionTest( QObject *parent = 0 );

    void setParent( const Akonadi::Collection &parent );
    void setCollection( const Akonadi::Collection &collection );

  public slots:
    void setParent( const QString &parentPath );
    void setCollection( const QString &path );
    void setName( const QString &name );
    void addContentType( const QString &type );

    void create();
    void update();
    void remove();

  private:
    Akonadi::Collection mParent;
    Akonadi::Collection mCollection;
};

#endif
