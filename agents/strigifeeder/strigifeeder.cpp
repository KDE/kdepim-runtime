/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "strigifeeder.h"

#include <akonadi/changerecorder.h>
#include <akonadi/item.h>

#include <kdebug.h>
#include <kurl.h>

#include <QDateTime>

using namespace Akonadi;

StrigiFeeder::StrigiFeeder(const QString & id) :
    AgentBase( id )
{
  changeRecorder()->monitorAll();
  changeRecorder()->addFetchPart( Item::PartBody );
  changeRecorder()->setChangeRecordingEnabled( false );
}

void StrigiFeeder::itemAdded(const Akonadi::Item & item, const Akonadi::Collection & collection)
{
  Q_UNUSED( collection );
  itemChanged( item, QStringList() );
}

void StrigiFeeder::itemChanged(const Akonadi::Item & item, const QStringList & partIdentifiers)
{
  Q_UNUSED( partIdentifiers );
  QByteArray data = item.part( Item::PartBody );
  mStrigi.indexFile( item.url().url(), QDateTime::currentDateTime().toTime_t(), data );
}

void StrigiFeeder::itemRemoved(const Akonadi::Item & item)
{
  mStrigi.indexFile( item.url().url(), QDateTime::currentDateTime().toTime_t(), QByteArray() );
}

AKONADI_AGENT_MAIN( StrigiFeeder )

#include "strigifeeder.moc"
