/* This file is part of the KDE project

   Copyright 2008 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "pimapplication.h"

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kstartupinfo.h>

#include <qdbusconnectioninterface.h>
#include <qdbusinterface.h>

using namespace KPIM;

PimApplication::PimApplication()
  : KUniqueApplication()
{
  // This object name is used in start(), and also in kontact's UniqueAppHandler.
  const QString objectName = QString( '/' ) + applicationName() + "_PimApplication";
  QDBusConnection::sessionBus().registerObject(
    objectName, this,
    QDBusConnection::ExportScriptableSlots |
    QDBusConnection::ExportScriptableProperties |
    QDBusConnection::ExportAdaptors );
}

static const char _k_sessionBusName[] = "kdepimapplication_session_bus";

static QDBusConnection tryToInitDBusConnection()
{
  // Check the D-Bus connection health, and connect - but not using QDBusConnection::sessionBus()
  // since we can't close that one before forking.
  QDBusConnection connection = QDBusConnection::connectToBus(
    QDBusConnection::SessionBus, _k_sessionBusName );
  if ( !connection.isConnected() ) {
    kError() << "Cannot find the D-Bus session server" << endl; //krazy:exclude=kdebug
    ::exit(255);
  }
  return connection;
}

bool PimApplication::start()
{
  const QString appName = KCmdLineArgs::aboutData()->appName();
  // Try talking to /appName_PimApplication in org.kde.appName,
  // (which could be kontact or the standalone application),
  // otherwise fall back to standard KUniqueApplication behavior (start appName).

  const QString serviceName = "org.kde." + appName;
  if ( tryToInitDBusConnection().interface()->isServiceRegistered( serviceName ) ) {
    QByteArray saved_args;
    QDataStream ds( &saved_args, QIODevice::WriteOnly );
    KCmdLineArgs::saveAppArgs( ds );

    QByteArray new_asn_id;
#if defined Q_WS_X11
    KStartupInfoId id;
    if( kapp ) { // KApplication constructor unsets the env. variable
      id.initId( kapp->startupId() );
    } else {
      id = KStartupInfo::currentStartupIdEnv();
    }
    if( !id.none() ) {
      new_asn_id = id.id();
    }
#endif

    const QString objectName = QString( '/' ) + appName + "_PimApplication";
    //kDebug() << objectName;
    QDBusInterface iface(
      serviceName, objectName, "org.kde.KUniqueApplication", QDBusConnection::sessionBus() );
    QDBusReply<int> reply;
    if ( iface.isValid() &&
         ( reply = iface.call( "newInstance", new_asn_id, saved_args ) ).isValid() ) {
      return false; // success means that main() can exit now.
    }
  }

  QDBusConnection::disconnectFromBus( _k_sessionBusName );

  //kDebug() << "kontact not running -- start standalone application";
  // kontact not running -- start standalone application.
  return KUniqueApplication::start();
}
