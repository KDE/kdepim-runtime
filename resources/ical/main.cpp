/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>

#include "icalresource.h"

using namespace PIM;

int main( int argc, char **argv )
{
  QCoreApplication app( argc, argv );
  QStringList args = QCoreApplication::arguments();
  if ( args.count() <= 1 ) {
    qDebug( "Unable to start agent: Missing resource identifier" );
    return 1;
  }

  ICalResource resource( args.last() );
  if ( !QDBus::sessionBus().registerService( "org.kde.Akonadi.Resource_" + args.last() ) ) {
    qDebug( "Unable to connect to dbus service: %s", qPrintable( QDBus::sessionBus().lastError().message() ) );
    return 1;
  }

  return app.exec();
}
