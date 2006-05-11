/*
  broadcaststatus.cpp

  This file is part of KDEPIM.

  Author: Don Sanders <sanders@kde.org>

  Copyright (C) 2000 Don Sanders <sanders@kde.org>

  License GPL
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QDateTime>

#include <klocale.h>
#include <kglobal.h>
#include <kstaticdeleter.h>

#include "broadcaststatus.h"
#include "progressmanager.h"

KPIM::BroadcastStatus* KPIM::BroadcastStatus::instance_ = 0;
static KStaticDeleter<KPIM::BroadcastStatus> broadcastStatusDeleter;

namespace KPIM {

BroadcastStatus* BroadcastStatus::instance()
{
  if (!instance_)
    broadcastStatusDeleter.setObject( instance_, new BroadcastStatus() );

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

void BroadcastStatus::setStatusMsg( const QString& message )
{
  mStatusMsg = message;
  if( !mTransientActive )
    emit statusMsg( message );
}

void BroadcastStatus::setStatusMsgWithTimestamp( const QString& message )
{
  KLocale* locale = KGlobal::locale();
  setStatusMsg( i18nc( "%1 is a time, %2 is a status message", "[%1] %2" ,
                  locale->formatTime( QTime::currentTime(),
                                          true /* with seconds */ ) ,
                  message ) );
}

void BroadcastStatus::setStatusMsgTransmissionCompleted( int numMessages,
                                                           int numBytes,
                                                           int numBytesRead,
                                                           int numBytesToRead,
                                                           bool mLeaveOnServer,
                                                           KPIM::ProgressItem* item )
{
  QString statusMsg;
  if( numMessages > 0 ) {
    if( numBytes != -1 ) {
      if( ( numBytesToRead != numBytes ) && mLeaveOnServer )
        statusMsg = i18np( "Transmission complete. %n new message in %1 KB "
                          "(%2 KB remaining on the server).",
                          "Transmission complete. %n new messages in %1 KB "
                          "(%2 KB remaining on the server).",
                          numMessages ,
                      numBytesRead / 1024 ,
                      numBytes / 1024 );
      else
        statusMsg = i18np( "Transmission complete. %n message in %1 KB.",
                         "Transmission complete. %n messages in %1 KB.",
                          numMessages ,
                      numBytesRead / 1024 );
    }
    else
      statusMsg = i18np( "Transmission complete. %n new message.",
                        "Transmission complete. %n new messages.",
                        numMessages );
  }
  else
    statusMsg = i18n( "Transmission complete. No new messages." );

  setStatusMsgWithTimestamp( statusMsg );
  if ( item )
    item->setStatus( statusMsg );
}

void BroadcastStatus::setStatusMsgTransmissionCompleted( const QString& account,
                                                           int numMessages,
                                                           int numBytes,
                                                           int numBytesRead,
                                                           int numBytesToRead,
                                                           bool mLeaveOnServer,
                                                           KPIM::ProgressItem* item )
{
  QString statusMsg;
  if( numMessages > 0 ) {
    if( numBytes != -1 ) {
      if( ( numBytesToRead != numBytes ) && mLeaveOnServer )
        statusMsg = i18np( "Transmission for account %3 complete. "
                          "%n new message in %1 KB "
                          "(%2 KB remaining on the server).",
                          "Transmission for account %3 complete. "
                          "%n new messages in %1 KB "
                          "(%2 KB remaining on the server).",
                          numMessages ,
                      numBytesRead / 1024 ,
                      numBytes / 1024 ,
                      account );
      else
        statusMsg = i18np( "Transmission for account %2 complete. "
                          "%n message in %1 KB.",
                          "Transmission for account %2 complete. "
                          "%n messages in %1 KB.",
                          numMessages ,
                      numBytesRead / 1024 ,
                      account );
    }
    else
      statusMsg = i18np( "Transmission for account %1 complete. "
                        "%n new message.",
                        "Transmission for account %1 complete. "
                        "%n new messages.",
                        numMessages ,
                    account );
  }
  else
    statusMsg = i18n( "Transmission for account %1 complete. No new messages.",
                  account );

  setStatusMsgWithTimestamp( statusMsg );
  if ( item )
    item->setStatus( statusMsg );
}

void BroadcastStatus::setTransientStatusMsg( const QString& msg )
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
