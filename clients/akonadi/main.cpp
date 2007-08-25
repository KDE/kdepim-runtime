/*
    This file is part of Akonadi.

    Copyright (c) 2006 Cornelius Schumacher <schumacher@kde.org>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "out.h"
#include "appendcommand.h"
#include "listcommand.h"
#include "fetchcommand.h"
#include "deletecommand.h"
#include "storecommand.h"

#include <kcomponentdata.h>

#include <QtCore/QCoreApplication>

int main( int argc, char **argv )
{
  QCoreApplication app( argc, argv );
  KComponentData kcd( "akonadi" );

  out() << "Akonadi Command Line Client (Version 0.1)" << endl;

  QString cmdarg;
  QString patharg;
  QString part;
  QString content;
  if ( argc >= 2 ) cmdarg = argv[ 1 ];
  if ( argc >= 3 ) patharg = argv[ 2 ];
  if ( argc >= 4 ) part = argv[ 3 ];
  if ( argc >= 5 ) content = argv[ 4 ];
  Command *cmd = 0;

  if ( cmdarg == "ls" ) {
    cmd = new ListCommand( patharg );
  }
  else if ( cmdarg == "fetch" ) {
    cmd = new FetchCommand( patharg, part );
  }
  else if ( cmdarg == "rm" ) {
    cmd = new DeleteCommand( patharg );
  }
  else if ( cmdarg == "store" ) {
    cmd = new StoreCommand( patharg, part, content );
  }
  else if ( cmdarg == "append" ) {
    cmd = new AppendCommand( patharg, part, content );
  }

  if ( !cmd ) {
    err() << "Unknown command: '" << cmdarg << "'" << endl;
  } else {
    cmd->exec();
    delete cmd;
  }
}
