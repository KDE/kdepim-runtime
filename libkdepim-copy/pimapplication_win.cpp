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

/* This is temporary implementation to address the 
  "dbus-daemon needs to be killed to start kontact again" issue on Windows.

  Description of the proposed workaround and the recipe is published at
  http://techbase.kde.org/Projects/PIM/MS_Windows#.22Locked.22_dbus-daemon 
*/

#include "pimapplication.h"

#include <windows.h>
#include <comdef.h> // (bstr_t)
#include <winperf.h>
#include <psapi.h>
#include <signal.h>
#include <unistd.h>

#include <qlist.h>
#include <qfile.h>
#include <qprocess.h>

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kdebug.h>

static PPERF_OBJECT_TYPE FirstObject( PPERF_DATA_BLOCK PerfData )
{
  return (PPERF_OBJECT_TYPE)((PBYTE)PerfData + PerfData->HeaderLength);
}

static PPERF_INSTANCE_DEFINITION FirstInstance( PPERF_OBJECT_TYPE PerfObj )
{
  return (PPERF_INSTANCE_DEFINITION)((PBYTE)PerfObj + PerfObj->DefinitionLength);
}

static PPERF_OBJECT_TYPE NextObject( PPERF_OBJECT_TYPE PerfObj )
{
  return (PPERF_OBJECT_TYPE)((PBYTE)PerfObj + PerfObj->TotalByteLength);
}

static PPERF_COUNTER_DEFINITION FirstCounter( PPERF_OBJECT_TYPE PerfObj )
{
  return (PPERF_COUNTER_DEFINITION) ((PBYTE)PerfObj + PerfObj->HeaderLength);
}

static PPERF_INSTANCE_DEFINITION NextInstance( PPERF_INSTANCE_DEFINITION PerfInst )
{
  PPERF_COUNTER_BLOCK PerfCntrBlk 
    = (PPERF_COUNTER_BLOCK)((PBYTE)PerfInst + PerfInst->ByteLength);
  return (PPERF_INSTANCE_DEFINITION)((PBYTE)PerfCntrBlk + PerfCntrBlk->ByteLength);
}

static PPERF_COUNTER_DEFINITION NextCounter( PPERF_COUNTER_DEFINITION PerfCntr )
{
  return (PPERF_COUNTER_DEFINITION)((PBYTE)PerfCntr + PerfCntr->ByteLength);
}

static PPERF_COUNTER_BLOCK CounterBlock(PPERF_INSTANCE_DEFINITION PerfInst)
{
  return (PPERF_COUNTER_BLOCK) ((LPBYTE) PerfInst + PerfInst->ByteLength);
}

#define GETPID_TOTAL 64 * 1024
#define GETPID_BYTEINCREMENT 1024
#define GETPID_PROCESS_OBJECT_INDEX 230
#define GETPID_PROC_ID_COUNTER 784

static void getProcessIdForName( const QString& processName, QList<int>& pids )
{
  LPCTSTR pProcessName = (LPCTSTR)processName.utf16();
  PPERF_OBJECT_TYPE perfObject;
  PPERF_INSTANCE_DEFINITION perfInstance;
  PPERF_COUNTER_DEFINITION perfCounter, curCounter;
  PPERF_COUNTER_BLOCK counterPtr;
  DWORD bufSize = GETPID_TOTAL;
  PPERF_DATA_BLOCK perfData = (PPERF_DATA_BLOCK) malloc( bufSize );

  char key[64];
  sprintf(key,"%d %d", GETPID_PROCESS_OBJECT_INDEX, GETPID_PROC_ID_COUNTER);
  LONG lRes;
  while( (lRes = RegQueryValueExA( HKEY_PERFORMANCE_DATA,
                               key,
                               NULL,
                               NULL,
                               (LPBYTE) perfData,
                               &bufSize )) == ERROR_MORE_DATA )
  {
    // get a buffer that is big enough
    bufSize += GETPID_BYTEINCREMENT;
    perfData = (PPERF_DATA_BLOCK) realloc( perfData, bufSize );
  }

  // Get the first object type.
  perfObject = FirstObject( perfData );

  // Process all objects.
  for( uint i = 0; i < perfData->NumObjectTypes; i++ ) {
    if (perfObject->ObjectNameTitleIndex != GETPID_PROCESS_OBJECT_INDEX) {
      perfObject = NextObject( perfObject );
      continue;
    }
    pids.clear();
    perfCounter = FirstCounter( perfObject );
    perfInstance = FirstInstance( perfObject );
    _bstr_t bstrProcessName,bstrInput;
    // retrieve the instances
    for( int instance = 0; instance < perfObject->NumInstances; instance++ ) {
      curCounter = perfCounter;
      bstrInput = pProcessName;
      bstrProcessName = (wchar_t *)((PBYTE)perfInstance + perfInstance->NameOffset);
      if (!_wcsicmp((LPCWSTR)bstrProcessName, (LPCWSTR) bstrInput)) {
        // retrieve the counters
        for( uint counter = 0; counter < perfObject->NumCounters; counter++ ) {
          if (curCounter->CounterNameTitleIndex == GETPID_PROC_ID_COUNTER) {
            counterPtr = CounterBlock(perfInstance);
            DWORD *value = (DWORD*)((LPBYTE) counterPtr + curCounter->CounterOffset);
            pids.append( int( *value ) );
            break;
          }
          curCounter = NextCounter( curCounter );
        }
      }
      perfInstance = NextInstance( perfInstance );
    }
  }
  free(perfData);
  RegCloseKey(HKEY_PERFORMANCE_DATA);
}

static bool processesExist(const QString& processName)
{
  if ( processName != "kontact" ) {
    QList<int> kontactPids;
    getProcessIdForName( QLatin1String("kontact"), kontactPids );
    if ( ! kontactPids.isEmpty() ) {
      kDebug() << "kontact process exitsts:" << kontactPids[0];
      return true;
    }
  }
  QList<int> pids;
  getProcessIdForName( processName, pids );
  int myPid = getpid();
  foreach ( int pid, pids ) {
    if (myPid != pid) {
      kDebug() << "Process ID is " << pid;
      return true;
    }
  }
  return false;
}

void killProcess(const QString& processName)
{
  QList<int> pids;
  getProcessIdForName( processName, pids );
  if ( pids.empty() )
    return;
  qWarning() << "Killing process \"" << processName << " (pid=" << pids[0] << ")..";
  int result;
  result = kill( pids[0], SIGTERM );
  if (result != 0)
    result = kill( pids[0], SIGKILL );
}

static void cleanupDBusMessAndRestart()
{
  if ( !processesExist( KCmdLineArgs::aboutData()->appName() ) ) {
    killProcess( "dbus-daemon" );
    QProcess proc;
    QStringList args;
    for ( int i = 1; i < KCmdLineArgs::qtArgc(); i++ )
      args += QFile::decodeName( KCmdLineArgs::qtArgv()[i] );
    qWarning() << "Restarting process \"" << KCmdLineArgs::qtArgv()[0] << "\"..";
    proc.startDetached( QFile::decodeName( KCmdLineArgs::qtArgv()[0] ), 
      args, KCmdLineArgs::cwd() );
  }
}

//------------

using namespace KPIM;

PimApplication::PimApplication()
  : KUniqueApplication()
{
}

/*static*/
bool PimApplication::start()
{
  bool result = KUniqueApplication::start();
  if ( ! result )
    cleanupDBusMessAndRestart();
  return result;
}
