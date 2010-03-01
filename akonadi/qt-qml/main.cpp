/*
* This file is part of Akonadi
*
* Copyright 2010 Stephen Kelly <steveire@gmail.com>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
* 02110-1301  USA
*/
#if 0
#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#endif
#include <QApplication>

#include "contactswidget.h"
#include "calendarwidget.h"


int main( int argc, char **argv )
{
  QApplication app(argc, argv);
#if 0
  const QByteArray& ba = QByteArray( "akonadi_qml" );
  const KLocalizedString name = ki18n( "Akonadi Qml example" );
  KAboutData aboutData( ba, ba, name, ba, name );
  KCmdLineArgs::init( argc, argv, &aboutData );
  KApplication app;
  if ( KApplication::instance()->arguments().contains("contacts") )
  {
    ContactsWidget contactsWidget;
    contactsWidget.show();
  }

  if ( KApplication::instance()->arguments().contains("calendar") )
  {
    #endif
    ContactsWidget calendarWidget;
    calendarWidget.show();
//   }
  return app.exec();
}
