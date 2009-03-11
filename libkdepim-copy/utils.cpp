/**
 * utils.cpp
 *
 * Copyright (C) 2007 Laurent Montel <montel@kde.org>
 * Copyright (C) 2008 Jarosław Staniek <staniek@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "utils.h"

using namespace KPIM;

#ifdef Q_WS_WIN

#include <windows.h>
#include <winperf.h>
#include <psapi.h>
#include <signal.h>
#include <unistd.h>

#include <QtCore/QList>
#include <QtCore/QtDebug>

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

static QString fromWChar(const wchar_t *string, int size = -1)
{
  return (sizeof(wchar_t) == sizeof(QChar)) ? QString::fromUtf16((ushort *)string, size)
    : QString::fromUcs4((uint *)string, size);
}

void Utils::getProcessesIdForName( const QString& processName, QList<int>& pids )
{
  qDebug() << "Utils::getProcessesIdForName" << processName;
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
  if ( !perfObject ) return;

  // Process all objects.
  for( uint i = 0; i < perfData->NumObjectTypes; i++ ) {
    if (perfObject->ObjectNameTitleIndex != GETPID_PROCESS_OBJECT_INDEX) {
      perfObject = NextObject( perfObject );
      continue;
    }
    pids.clear();
    perfCounter = FirstCounter( perfObject );
    perfInstance = FirstInstance( perfObject );
    // retrieve the instances
    qDebug() << "INSTANCES: " << perfObject->NumInstances;
    for( int instance = 0; instance < perfObject->NumInstances; instance++ ) {
      curCounter = perfCounter;
      const QString foundProcessName(
        fromWChar( (wchar_t *)( (PBYTE)perfInstance + perfInstance->NameOffset ) ) );
      qDebug() << "foundProcessName: " << foundProcessName;
      if ( foundProcessName == processName ) {
        // retrieve the counters
        for( uint counter = 0; counter < perfObject->NumCounters; counter++ ) {
          if (curCounter->CounterNameTitleIndex == GETPID_PROC_ID_COUNTER) {
            counterPtr = CounterBlock(perfInstance);
            DWORD *value = (DWORD*)((LPBYTE) counterPtr + curCounter->CounterOffset);
            pids.append( int( *value ) );
            qDebug() << "found PID: " << int( *value );
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

bool Utils::otherProcessesExist( const QString& processName )
{
  QList<int> pids;
  getProcessesIdForName( processName, pids );
  int myPid = getpid();
  foreach ( int pid, pids ) {
    if (myPid != pid) {
//      kDebug() << "Process ID is " << pid;
      return true;
    }
  }
  return false;
}

bool Utils::killProcesses( const QString& processName )
{
  QList<int> pids;
  getProcessesIdForName( processName, pids );
  if ( pids.empty() )
    return true;
  qWarning() << "Killing process \"" << processName << " (pid=" << pids[0] << ")..";
  int overallResult = 0;
  foreach( int pid, pids ) {
    int result = kill( pid, SIGTERM );
    if ( result == 0 )
      continue;
    result = kill( pid, SIGKILL );
    if ( result != 0 )
      overallResult = result;
  }
  return overallResult == 0;
}

struct EnumWindowsStruct
{
  EnumWindowsStruct() : windowId( 0 ) {}
  int pid;
  HWND windowId;
};


BOOL CALLBACK EnumWindowsProc( HWND hwnd, LPARAM lParam )

{

  if ( GetWindowLong( hwnd, GWL_STYLE ) & WS_VISIBLE ) {

    DWORD pidwin;

    GetWindowThreadProcessId(hwnd, &pidwin);
    if ( pidwin == ((EnumWindowsStruct*)lParam)->pid ) {
      ((EnumWindowsStruct*)lParam)->windowId = hwnd;
      return FALSE; //krazy:exclude=captruefalse
    }
  }
  return TRUE; //krazy:exclude=captruefalse
}

void Utils::activateWindowForProcess( const QString& executableName )
{
  QList<int> pids;
  KPIM::Utils::getProcessesIdForName( executableName, pids );
  int myPid = getpid();
  int foundPid = 0;
  foreach ( int pid, pids ) {
    if (myPid != pid) {
      qDebug() << "activateWindowForProcess(): PID to activate:" << pid;
      foundPid = pid;
      break;
    }
  }
  if ( foundPid == 0 )
    return;
  EnumWindowsStruct winStruct;
  winStruct.pid = foundPid;
  EnumWindows( EnumWindowsProc, (LPARAM)&winStruct );
  if ( winStruct.windowId == NULL )
    return;
  SetForegroundWindow( winStruct.windowId );
}

#endif // Q_WS_WIN

QString Utils::rot13(const QString &s)
{
  QString r(s);

  for (int i=0; i<r.length(); i++) {
    if ( r[i] >= QChar('A') && r[i] <= QChar('M') ||
         r[i] >= QChar('a') && r[i] <= QChar('m') )
      r[i] = (char)((int)QChar(r[i]).toLatin1() + 13);
    else
      if  ( r[i] >= QChar('N') && r[i] <= QChar('Z') ||
            r[i] >= QChar('n') && r[i] <= QChar('z') )
        r[i] = (char)((int)QChar(r[i]).toLatin1() - 13);
  }

  return r;
}
