/*
    This file is part of libkdepim.

    Copyright (c) 2004 Daniel Molkentin <danimo@klaralvdalens-datakonsult.se>

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

#include <kaboutdata.h>
#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kcmdlineargs.h>

#include "../addresseeselector.h"
#include "../addresseeemailselection.h"

int main( int argc, char **argv )
{
  KAboutData aboutData( "testaddresseeseletor", "Test AddresseeSelector", "0.1" );
  KCmdLineArgs::init( argc, argv, &aboutData );

  KApplication app;

  KPIM::AddresseeEmailSelection selection;

  KPIM::AddresseeSelectorDialog dlg( &selection );
  dlg.exec();

  kdDebug() << "to: " << selection.to() << endl;
  kdDebug() << "cc: " << selection.cc() << endl;
  kdDebug() << "bcc: " << selection.bcc() << endl;

  kdDebug() << "toDistlists: " << selection.toDistributionLists() << endl;
  kdDebug() << "ccDistlists: " << selection.ccDistributionLists() << endl;
  kdDebug() << "bccDistlists: " << selection.bccDistributionLists() << endl;

  return 0;
}
