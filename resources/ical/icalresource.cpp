/*
    Copyright (c) 2006 Till Adam <adam@kde.org>

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
#include "settingsadaptor.h"
#include "singlefileresourceconfigdialog.h"
#include "kcal/kcalmimetypevisitor.h"  // the kcal at the akonadi top-level

#include <kcal/assignmentvisitor.h>
#include <kcal/calendarlocal.h>
#include <kcal/incidence.h>

#include <kdebug.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kurl.h>

#include <boost/shared_ptr.hpp>

using namespace Akonadi;
using namespace KCal;

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

ICalResource::ICalResource( const QString &id )
    : SingleFileResource<Settings>( id ), mCalendar( 0 ),
      mMimeVisitor( new KCalMimeTypeVisitor() ),
      mIncidenceAssigner( new AssignmentVisitor() ),
      mNotesMimeType( QLatin1String( "application/x-vnd.kde.notes" ) )
{
  QStringList mimeTypes;
  if ( isNotesResource() ) {
    mimeTypes << mNotesMimeType;
    setSupportedMimetypes( mimeTypes, "knotes" );
  }
  else {
    mimeTypes << QLatin1String( "text/calendar" );
    mimeTypes += mMimeVisitor->allMimeTypes();
    setSupportedMimetypes( mimeTypes, "office-calendar" );
  }


  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );
}

ICalResource::~ICalResource()
{
  delete mCalendar;
  delete mMimeVisitor;
  delete mIncidenceAssigner;
}

bool ICalResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );
  kDebug( 5251 ) << "Item:" << item.url();

  if ( !mCalendar ) {
    emit error( i18n("Calendar not loaded.") );
    return false;
  }

  const QString rid = item.remoteId();
  Incidence* incidence = mCalendar->incidence( rid );
  if ( !incidence ) {
    emit error( i18n("Incidence with uid '%1' not found.", rid ) );
    return false;
  }

  IncidencePtr incidencePtr( incidence->clone() );

  Item i = item;
  QString mimeType;
  if ( isNotesResource() )
    mimeType = mNotesMimeType;
  else
    mimeType = mMimeVisitor->mimeType( incidencePtr.get() );

  i.setMimeType( mimeType );
  i.setPayload<IncidencePtr>( incidencePtr );
  itemRetrieved( i );
  return true;
}

void ICalResource::aboutToQuit()
{
  if ( !Settings::self()->readOnly() )
    writeFile();

  Settings::self()->writeConfig();
}

void ICalResource::configure( WId windowId )
{
  if ( isNotesResource() )
    Settings::self()->setPath( KGlobal::dirs()->saveLocation( "data", "knotes/" ) + "notes.ics" );

  SingleFileResourceConfigDialog<Settings> dlg( windowId );
  dlg.setFilter( "*.ics *.ical|" + i18nc("Filedialog filter for *.ics *.ical", "iCal Calendar File" ) );
  dlg.setCaption( i18n("Select Calendar") );
  if ( dlg.exec() == QDialog::Accepted ) {
    reloadFile();
  }
}

bool ICalResource::readFromFile( const QString &fileName )
{
  delete mCalendar;
  mCalendar = 0;

  mCalendar = new KCal::CalendarLocal( QLatin1String( "UTC" ) );
  mCalendar->load( fileName );
  return true;
}

void ICalResource::itemAdded( const Akonadi::Item & item, const Akonadi::Collection& )
{
  if ( !mCalendar ) {
    cancelTask( i18n("Calendar not loaded.") );
    return;
  }

  if ( !item.hasPayload<IncidencePtr>() ) {
    cancelTask( i18n("Unable to retrieve added item %1.").arg( item.id() ) );
    return;
  }

  IncidencePtr i = item.payload<IncidencePtr>();
  mCalendar->addIncidence( i.get()->clone() );
  Item it( item );
  it.setRemoteId( i->uid() );
  fileDirty();
  changeCommitted( it );
}

void ICalResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts )

  if ( !mCalendar ) {
    cancelTask( i18n("Calendar not loaded.") );
    return;
  }

  if ( !item.hasPayload<IncidencePtr>() ) {
    cancelTask( i18n("Unable to retrieve modified item %1.").arg( item.id() ) );
    return;
  }

  IncidencePtr payload = item.payload<IncidencePtr>();
  Incidence *incidence = mCalendar->incidence( item.remoteId() );
  if ( !incidence ) {
    // not in the calendar yet, should not happen -> add it
    mCalendar->addIncidence( payload.get()->clone() );
  } else {
    // make sure any observer the resource might have installed gets properly notified
    incidence->startUpdates();
    bool assignResult = mIncidenceAssigner->assign( incidence, payload.get() );
    if ( assignResult )
      incidence->updated();
    incidence->endUpdates();

    if ( !assignResult ) {
      kWarning() << "Item changed incidence type. Replacing it.";

      mCalendar->deleteIncidence( incidence );
      delete incidence;
      mCalendar->addIncidence( payload.get()->clone() );
    }
  }
  fileDirty();
  changeCommitted( item );
}

void ICalResource::itemRemoved(const Akonadi::Item & item)
{
  if ( !mCalendar ) {
    cancelTask( i18n("Calendar not loaded.") );
    return;
  }

  Incidence *i = mCalendar->incidence( item.remoteId() );
  if ( i )
    mCalendar->deleteIncidence( i );
  fileDirty();
  changeProcessed();
}

void ICalResource::retrieveItems( const Akonadi::Collection & col )
{
  Q_UNUSED( col );
  if ( !mCalendar )
    return;

  Incidence::List incidences = mCalendar->incidences();
  Item::List items;
  foreach ( Incidence *incidence, incidences ) {
    QString mimeType;
    if ( isNotesResource() )
      mimeType = mNotesMimeType;
    else
      mimeType = mMimeVisitor->mimeType( incidence );
    Item item ( mimeType );
    item.setRemoteId( incidence->uid() );
    item.setPayload( IncidencePtr( incidence->clone() ) );
    items << item;
  }
  itemsRetrieved( items );
}

bool ICalResource::writeToFile( const QString &fileName )
{
  if ( !mCalendar->save( fileName ) ) {
    emit error( i18n("Failed to save calendar file to %1", fileName ) );
    return false;
  }
  return true;
}

bool ICalResource::isNotesResource() const
{
  return identifier().startsWith("akonadi_notes");
}

AKONADI_RESOURCE_MAIN( ICalResource )


#include "icalresource.moc"
