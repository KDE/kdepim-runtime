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

#ifndef TEST_H
#define TEST_H

#ifdef assert
  #undef assert
#endif

#include <QObject>

class Test : public QObject
{
  Q_OBJECT
  public:
    static Test* instance();

  public slots:
    void assert( bool value );
    void verify( QObject *object, const QString &slot );
    void fail( const QString &error );
    void abort();
    void alert( const QString &msg );

  private:
    Test( QObject *parent = 0 );

    static Test* mSelf;
};

#endif
