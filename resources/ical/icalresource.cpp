/*
    Copyright (c) 2006 Till Adam <adam@kde.org>
    Copyright (c) 2009 David Jarvie <djarvie@kde.org>

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

#include "icalresource.h"

#include <kcalcore/memorycalendar.h>
#include <kcalcore/freebusy.h>

#include <kdebug.h>
#include <klocale.h>

using namespace Akonadi;
using namespace KCalCore;

ICalResource::ICalResource( const QString &id )
    : ICalResourceBase( id )
{
  QStringList mimeTypes;
  mimeTypes << QLatin1String( "text/calendar" );
  mimeTypes += allMimeTypes();
  initialise( mimeTypes, "office-calendar" );
}

ICalResource::ICalResource( const QString &id, const QStringList &mimeTypes, const QString& icon )
    : ICalResourceBase( id )
{
  initialise( mimeTypes, icon );
}

ICalResource::~ICalResource()
{
}

bool ICalResource::doRetrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );
  const QString rid = item.remoteId();
  Incidence::Ptr incidence = calendar()->incidence( rid );
  if ( !incidence ) {
    emit error( i18n("Incidence with uid '%1' not found.", rid ) );
    return false;
  }

  Incidence::Ptr incidencePtr( incidence->clone() );

  Item i = item;
  i.setMimeType( incidencePtr->mimeType() );
  i.setPayload<Incidence::Ptr>( incidencePtr );
  itemRetrieved( i );
  return true;
}

void ICalResource::itemAdded( const Akonadi::Item & item, const Akonadi::Collection& )
{
  if ( !checkItemAddedChanged<Incidence::Ptr>( item, CheckForAdded ) ) {
    return;
  }

  Incidence::Ptr i = item.payload<Incidence::Ptr>();
  if ( !calendar()->addIncidence( Incidence::Ptr( i->clone() ) ) ) {
    cancelTask();
    return;
  }

  Item it( item );
  it.setRemoteId( i->uid() );
  scheduleWrite();
  changeCommitted( it );
}

void ICalResource::itemChanged( const Akonadi::Item &item,
                                const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts )

    if ( !checkItemAddedChanged<Incidence::Ptr>( item, CheckForChanged ) ) {
    return;
  }

  Incidence::Ptr payload = item.payload<Incidence::Ptr>();
  Incidence::Ptr incidence = calendar()->incidence( item.remoteId() );
  if ( !incidence ) {
    // not in the calendar yet, should not happen -> add it
    calendar()->addIncidence( Incidence::Ptr( payload->clone() ) );
  } else {
    // make sure any observer the resource might have installed gets properly notified
    incidence->startUpdates();

    if ( incidence->type() == payload->type() ) {
      // IncidenceBase::operator= calls virtual method assign, so it's safe.
      *incidence.staticCast<IncidenceBase>().data() = *payload.data();
      incidence->updated();
      incidence->endUpdates();
    } else {
      incidence->endUpdates();
      kWarning() << "Item changed incidence type. Replacing it.";

      calendar()->deleteIncidence( incidence );
      calendar()->addIncidence( Incidence::Ptr( payload->clone() ) );
    }
  }
  scheduleWrite();
  changeCommitted( item );
}

void ICalResource::doRetrieveItems( const Akonadi::Collection & col )
{
  Q_UNUSED( col );
  Incidence::List incidences = calendar()->incidences();
  Item::List items;
  foreach ( const Incidence::Ptr &incidence, incidences ) {
    Item item ( incidence->mimeType() );
    item.setRemoteId( incidence->uid() );
    item.setPayload( Incidence::Ptr( incidence->clone() ) );
    items << item;
  }
  itemsRetrieved( items );
}

QStringList ICalResource::allMimeTypes() const
{
  return QStringList() << KCalCore::Event::eventMimeType()
                       << KCalCore::Todo::todoMimeType()
                       << KCalCore::Journal::journalMimeType()
                       << KCalCore::FreeBusy::freeBusyMimeType();
}

QString ICalResource::mimeType( const IncidenceBase::Ptr &incidence ) const
{
  return incidence->mimeType();
}


#include "icalresource.moc"
