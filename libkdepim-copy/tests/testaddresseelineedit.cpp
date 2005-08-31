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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <kaboutdata.h>
#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kcmdlineargs.h>

#include "../addresseelineedit.h"

int main(int argc, char* argv[])
{
  KAboutData aboutData("testaddresseelineedit","Test AddresseeLineEdit","0.1");
  KCmdLineArgs::init(argc,argv,&aboutData);

  KApplication app;

  KPIM::AddresseeLineEdit *kale = new KPIM::AddresseeLineEdit(0);
  kale->resize( 400, 20 );
  kale->show();

  return app.exec();

}
    
