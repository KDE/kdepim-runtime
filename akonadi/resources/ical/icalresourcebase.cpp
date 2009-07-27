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

#include "icalresourcebase.h"
#include "settingsadaptor.h"
#include "singlefileresourceconfigdialog.h"

#include <kcal/calendarlocal.h>
#include <kcal/incidence.h>

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>

using namespace Akonadi;
using namespace KCal;

ICalResourceBase::ICalResourceBase( const QString &id, const QString &fileTypeDescription )
    : SingleFileResource<Settings>( id ),
      mCalendar( 0 ),
      mFileDescription( fileTypeDescription )
{
  KGlobal::locale()->insertCatalog( "akonadi_ical_resource" );
}

void ICalResourceBase::initialise( const QStringList &mimeTypes, const QString &icon )
{
  setSupportedMimetypes( mimeTypes, icon );
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );
}

ICalResourceBase::~ICalResourceBase()
{
  delete mCalendar;
}

bool ICalResourceBase::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  kDebug( 5251 ) << "Item:" << item.url();

  if ( !mCalendar ) {
    emit error( i18n("Calendar not loaded.") );
    return false;
  }

  return doRetrieveItem( item, parts );
}

void ICalResourceBase::aboutToQuit()
{
  if ( !Settings::self()->readOnly() )
    writeFile();
  Settings::self()->writeConfig();
}

void ICalResourceBase::configure( WId windowId )
{
  SingleFileResourceConfigDialog<Settings> dlg( windowId );
  dlg.setFilter( "*.ics *.ical|" + mFileDescription );
  dlg.setCaption( i18n("Select Calendar") );
  if ( dlg.exec() == QDialog::Accepted ) {
    reloadFile();
  }
}

bool ICalResourceBase::readFromFile( const QString &fileName )
{
  delete mCalendar;
  mCalendar = new KCal::CalendarLocal( QLatin1String( "UTC" ) );
  mCalendar->load( fileName );
  return true;
}

void ICalResourceBase::itemRemoved(const Akonadi::Item & item)
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

void ICalResourceBase::retrieveItems( const Akonadi::Collection & col )
{
  if ( !mCalendar )
    return;
  doRetrieveItems( col );
}

bool ICalResourceBase::writeToFile( const QString &fileName )
{
  if ( !mCalendar->save( fileName ) ) {
    emit error( i18n("Failed to save calendar file to %1", fileName ) );
    return false;
  }
  return true;
}

KCal::CalendarLocal *ICalResourceBase::calendar() const
{
  return mCalendar;
}


#include "icalresourcebase.moc"
