/*
    This file is part of libkdepim.

    Copyright (C) 2008 Jaroslaw Staniek <js@iidea.pl>

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

#include "pimapplication.h"
#include "utils.h"

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kdebug.h>

#include <qprocess.h>
#include <qfile.h>

/* This is temporary implementation to address the 
  "dbus-daemon needs to be killed to start kontact again" issue on Windows.


  Description of the proposed workaround and the recipe is published at
  http://techbase.kde.org/Projects/PIM/MS_Windows#.22Locked.22_dbus-daemon 
*/

static void cleanupDBusMessAndRestart()
{
  if ( KCmdLineArgs::aboutData()->appName() != "kontact" ) {
    QList<int> kontactPids;
    KPIM::Utils::getProcessesIdForName( QLatin1String("kontact"), kontactPids );
    if ( ! kontactPids.isEmpty() ) {
      kDebug() << "kontact process exitsts:" << kontactPids[0];
      return;
    }
  }

  if ( !KPIM::Utils::otherProcessesExist( KCmdLineArgs::aboutData()->appName() ) ) {
    KPIM::Utils::killProcesses( "dbus-daemon" );
/*    QProcess proc;
    QStringList args;
    for ( int i = 1; i < KCmdLineArgs::qtArgc(); i++ )
      args += QFile::decodeName( KCmdLineArgs::qtArgv()[i] );
    kWarning() << "Restarting process \"" << KCmdLineArgs::qtArgv()[0] << "\"..";
    proc.startDetached( QFile::decodeName( KCmdLineArgs::qtArgv()[0] ), 
      args, KCmdLineArgs::cwd() );*/
  }
}

//------------

using namespace KPIM;

#include <process.h>
#include <windows.h>

PimApplication::PimApplication()
  : KUniqueApplication()
{
#if 0 // for testing KUniqueAppliaction on Windows
  MessageBoxA(NULL, 
             QString("PimApplication::PimApplication() %1 pid=%2").arg(applicationName()).arg(getpid()).toLatin1(), 
             QString("PimApplication \"%1\"").arg(applicationName()).toLatin1(), MB_OK|MB_ICONINFORMATION|MB_TASKMODAL);
#endif
}

/*static*/
bool PimApplication::start()
{
  bool result = KUniqueApplication::start();
  if ( ! result ) {
    cleanupDBusMessAndRestart();
    result = KUniqueApplication::start();
  }
  return result;
}
