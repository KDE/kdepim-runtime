/*
    This file is part of libkdepim.

    Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include <qapplication.h>

#include "idmapper.h"

int main( int argc, char **argv )
{
  QApplication app( argc, argv );

  KPIM::IdMapper mapper;

  mapper.setRemoteId( "foo", "bar" );
  mapper.setRemoteId( "yes", "klar" );
  mapper.setRemoteId( "no", "nee" );

  qDebug( "full:\n%s", mapper.asString().latin1() );

  mapper.save( "test.uidmap" );

  mapper.clear();
  qDebug( "empty:\n%s", mapper.asString().latin1() );

  mapper.load( "test.uidmap" );
  qDebug( "full again:\n%s", mapper.asString().latin1() );

  mapper.save( "test.uidmap" );

  mapper.clear();
  qDebug( "empty:\n%s", mapper.asString().latin1() );

  mapper.load( "test.uidmap" );
  qDebug( "full again:\n%s", mapper.asString().latin1() );
  return 0;
}
