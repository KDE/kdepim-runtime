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
#include "kcal/kcalmimetypevisitor.h"  // the kcal at the akonadi top-level

#include <kcal/assignmentvisitor.h>
#include <kcal/calendarlocal.h>
#include <kcal/incidence.h>

#include <kdebug.h>
#include <klocale.h>

#include <boost/shared_ptr.hpp>

using namespace Akonadi;
using namespace KCal;

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

ICalResource::ICalResource( const QString &id )
    : ICalResourceBase( id, i18nc("Filedialog filter for *.ics *.ical", "iCal Calendar File" ) ),
      mMimeVisitor( new KCalMimeTypeVisitor ),
      mIncidenceAssigner( new AssignmentVisitor() )
{
  QStringList mimeTypes;
  mimeTypes << QLatin1String( "text/calendar" );
  mimeTypes += allMimeTypes();
  initialise( mimeTypes, "office-calendar" );
}

ICalResource::ICalResource( const QString &id, const QStringList &mimeTypes, const QString& icon )
    : ICalResourceBase( id, i18nc("Filedialog filter for *.ics *.ical", "iCal Calendar File" ) ),
      mMimeVisitor( new KCalMimeTypeVisitor ),
      mIncidenceAssigner( new AssignmentVisitor() )
{
  initialise( mimeTypes, icon );
}

ICalResource::~ICalResource()
{
  delete mMimeVisitor;
  delete mIncidenceAssigner;
}

bool ICalResource::doRetrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );
  const QString rid = item.remoteId();
  Incidence* incidence = calendar()->incidence( rid );
  if ( !incidence ) {
    emit error( i18n("Incidence with uid '%1' not found.", rid ) );
    return false;
  }

  IncidencePtr incidencePtr( incidence->clone() );

  Item i = item;
  i.setMimeType( mimeType( incidencePtr.get() ) );
  i.setPayload<IncidencePtr>( incidencePtr );
  itemRetrieved( i );
  return true;
}

void ICalResource::itemAdded( const Akonadi::Item & item, const Akonadi::Collection& )
{
  if ( !checkItemAddedChanged<IncidencePtr>( item, CheckForAdded ) ) {
    return;
  }

  IncidencePtr i = item.payload<IncidencePtr>();
  calendar()->addIncidence( i.get()->clone() );
  Item it( item );
  it.setRemoteId( i->uid() );
  fileDirty();
  changeCommitted( it );
}

void ICalResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts )

  if ( !checkItemAddedChanged<IncidencePtr>( item, CheckForChanged ) ) {
    return;
  }

  IncidencePtr payload = item.payload<IncidencePtr>();
  Incidence *incidence = calendar()->incidence( item.remoteId() );
  if ( !incidence ) {
    // not in the calendar yet, should not happen -> add it
    calendar()->addIncidence( payload.get()->clone() );
  } else {
    // make sure any observer the resource might have installed gets properly notified
    incidence->startUpdates();
    bool assignResult = mIncidenceAssigner->assign( incidence, payload.get() );
    if ( assignResult )
      incidence->updated();
    incidence->endUpdates();

    if ( !assignResult ) {
      kWarning() << "Item changed incidence type. Replacing it.";

      calendar()->deleteIncidence( incidence );
      calendar()->addIncidence( payload.get()->clone() );
    }
  }
  fileDirty();
  changeCommitted( item );
}

void ICalResource::doRetrieveItems( const Akonadi::Collection & col )
{
  Q_UNUSED( col );
  Incidence::List incidences = calendar()->incidences();
  Item::List items;
  foreach ( Incidence *incidence, incidences ) {
    Item item ( mimeType( incidence ) );
    item.setRemoteId( incidence->uid() );
    item.setPayload( IncidencePtr( incidence->clone() ) );
    items << item;
  }
  itemsRetrieved( items );
}

QStringList ICalResource::allMimeTypes() const
{
    return mMimeVisitor->allMimeTypes();
}

QString ICalResource::mimeType( IncidenceBase *incidence )
{
  return mMimeVisitor->mimeType( incidence );
}

#include "icalresource.moc"
