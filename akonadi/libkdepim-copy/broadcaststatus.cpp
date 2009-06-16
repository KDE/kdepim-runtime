/*
  broadcaststatus.cpp

  This file is part of libkdepim.

  Author: Don Sanders <sanders@kde.org>

  Copyright (C) 2000 Don Sanders <sanders@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "broadcaststatus.h"
#include "progressmanager.h"

#include <KLocale>
#include <KGlobal>

#include <QCoreApplication>
#include <QTime>

static KPIM::BroadcastStatus *instance_;

static void deleteGlobalStaticBroadcastStatus()
{
  delete instance_;
  instance_ = 0;
}

namespace KPIM {

BroadcastStatus *BroadcastStatus::instance()
{
  if ( !instance_ ) {
    instance_ = new BroadcastStatus();
    qAddPostRoutine( deleteGlobalStaticBroadcastStatus );
  }
  return instance_;
}

BroadcastStatus::BroadcastStatus()
  :mTransientActive( false )
{
}

BroadcastStatus::~BroadcastStatus()
{
  instance_ = 0;
}

void BroadcastStatus::setStatusMsg( const QString &message )
{
  mStatusMsg = message;
  if( !mTransientActive ) {
    emit statusMsg( message );
  }
}

void BroadcastStatus::setStatusMsgWithTimestamp( const QString &message )
{
  KLocale *locale = KGlobal::locale();
  setStatusMsg( i18nc( "%1 is a time, %2 is a status message", "[%1] %2",
                       locale->formatTime( QTime::currentTime(), true ), message ) );
}

void BroadcastStatus::setStatusMsgTransmissionCompleted( int numMessages,
                                                         int numBytes,
                                                         int numBytesRead,
                                                         int numBytesToRead,
                                                         bool mLeaveOnServer,
                                                         KPIM::ProgressItem *item )
{
  QString statusMsg;
  if ( numMessages > 0 ) {
    if ( numBytes != -1 ) {
      if ( ( numBytesToRead != numBytes ) && mLeaveOnServer ) {
        statusMsg = i18np( "Transmission complete. %1 new message in %2 KB "
                           "(%3 KB remaining on the server).",
                           "Transmission complete. %1 new messages in %2 KB "
                           "(%3 KB remaining on the server).",
                           numMessages,
                           numBytesRead / 1024,
                           numBytes / 1024 );
      } else {
        statusMsg = i18np( "Transmission complete. %1 message in %2 KB.",
                           "Transmission complete. %1 messages in %2 KB.",
                           numMessages,
                           numBytesRead / 1024 );
      }
    } else {
      statusMsg = i18np( "Transmission complete. %1 new message.",
                         "Transmission complete. %1 new messages.",
                         numMessages );
    }
  } else {
    statusMsg = i18n( "Transmission complete. No new messages." );
  }

  setStatusMsgWithTimestamp( statusMsg );

  if ( item ) {
    item->setStatus( statusMsg );
  }
}

void BroadcastStatus::setStatusMsgTransmissionCompleted( const QString &account,
                                                         int numMessages,
                                                         int numBytes,
                                                         int numBytesRead,
                                                         int numBytesToRead,
                                                         bool mLeaveOnServer,
                                                         KPIM::ProgressItem *item )
{
  QString statusMsg;
  if ( numMessages > 0 ) {
    if ( numBytes != -1 ) {
      if ( ( numBytesToRead != numBytes ) && mLeaveOnServer ) {
        statusMsg = i18np( "Transmission for account %4 complete. "
                           "%1 new message in %2 KB "
                           "(%3 KB remaining on the server).",
                           "Transmission for account %4 complete. "
                           "%1 new messages in %2 KB "
                           "(%3 KB remaining on the server).",
                           numMessages,
                      numBytesRead / 1024,
                      numBytes / 1024,
                      account );
      } else {
        statusMsg = i18np( "Transmission for account %3 complete. "
                           "%1 message in %2 KB.",
                           "Transmission for account %3 complete. "
                           "%1 messages in %2 KB.",
                           numMessages,
                           numBytesRead / 1024,
                           account );
      }
    } else {
      statusMsg = i18np( "Transmission for account %2 complete. "
                         "%1 new message.",
                         "Transmission for account %2 complete. "
                         "%1 new messages.",
                         numMessages,
                         account );
    }
  } else {
    statusMsg = i18n( "Transmission for account %1 complete. No new messages.", account );
  }

  setStatusMsgWithTimestamp( statusMsg );

  if ( item ) {
    item->setStatus( statusMsg );
  }
}

void BroadcastStatus::setTransientStatusMsg( const QString &msg )
{
  mTransientActive = true;
  emit statusMsg( msg );
}

void BroadcastStatus::reset()
{
  mTransientActive = false;
  // restore
  emit statusMsg( mStatusMsg );
}

}

#include "broadcaststatus.moc"
