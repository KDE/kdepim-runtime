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
#include "listcommand.h"
#include "fetchcommand.h"
#include "deletecommand.h"

#include <kcomponentdata.h>
#include <qapplication.h>

int main( int argc, char **argv )
{
  QApplication app( argc, argv );
  KComponentData kcd( "akonadi" );

  out() << "Akonadi Command Line Client (Version 0.1)" << endl;

  QString cmdarg;
  QString patharg;
  if ( argc >= 2 ) cmdarg = argv[ 1 ];
  if ( argc >= 3 ) patharg = argv[ 2 ];

  Command *cmd = 0;

  if ( cmdarg == "ls" ) {
    cmd = new ListCommand( patharg );
  }
  else if ( cmdarg == "fetch" ) {
    cmd = new FetchCommand( patharg );
  }
  else if ( cmdarg == "rm" ) {
    cmd = new DeleteCommand( patharg );
  }

  if ( !cmd ) {
    err() << "Unknown command: '" << cmdarg << "'" << endl;
  } else {
    cmd->exec();
    delete cmd;
  }
}
