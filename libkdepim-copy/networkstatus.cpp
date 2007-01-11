/*
    This file is part of libkdepim.

    Copyright (c) 2005 Tobias Koenig <tokoe@kde.org>

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

#include <kconfig.h>
#include <kglobal.h>
#include <kstaticdeleter.h>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>

#include "networkstatus_p.h"
#include "networkstatus.h"

using namespace KPIM;

static KStaticDeleter<NetworkStatus> networkStatusDeleter;
NetworkStatus *NetworkStatus::mSelf = 0;

NetworkStatusAdaptor::NetworkStatusAdaptor(QObject *parent)
  : QDBusAbstractAdaptor(parent)
{
  setAutoRelaySignals(true);
}

NetworkStatus::NetworkStatus()
  : QObject( 0 )
{
  new NetworkStatusAdaptor( this );
  QDBusConnection::sessionBus().registerObject("/", this, QDBusConnection::ExportAdaptors);
  KConfigGroup group( KGlobal::config(), "NetworkStatus" );
  if ( group.readEntry( "Online", true ) == true )
    mStatus = Online;
  else
    mStatus = Offline;

  QDBusConnection::sessionBus().connect( "org.kde.kded", "/", "org.kde.NetworkStatus",
                               "onlineStatusChanged", this, SLOT( onlineStatusChanged() ) );
}

NetworkStatus::~NetworkStatus()
{
  KConfigGroup group( KGlobal::config(), "NetworkStatus" );
  group.writeEntry( "Online", mStatus == Online );
}

void NetworkStatus::setStatus( Status status )
{
  mStatus = status;

  emit statusChanged( mStatus );
}

NetworkStatus::Status NetworkStatus::status() const
{
  return mStatus;
}

void NetworkStatus::onlineStatusChanged()
{
#ifdef __GNUC__
#warning "kde4: verify it"	
#endif
  QDBusInterface call( "org.kde.kded", "/", "org.kde.kded" );
  QDBusMessage reply = call.call( "onlineStatus", true );
  if ( reply.type() == QDBusMessage::ReplyMessage ) {
    int status = reply.arguments().at( 0 ).toInt();
    if ( status == 3 )
      setStatus( Online );
    else {
      if ( mStatus != Offline )
        setStatus( Offline );
    }
  }
}

NetworkStatus *NetworkStatus::self()
{
  if ( !mSelf )
    networkStatusDeleter.setObject( mSelf, new NetworkStatus );

  return mSelf;
}

#include "networkstatus_p.moc"
#include "networkstatus.moc"
