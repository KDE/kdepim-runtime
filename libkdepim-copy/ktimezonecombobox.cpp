/*
  Copyright (C) 2007 Bruno Virlet <bruno.virlet@gmail.com>

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

#include "ktimezonecombobox.h"

#include <kcal/calendar.h>
#include <kcal/icaltimezones.h>

#include <KDebug>
#include <KGlobal>
#include <KLocale>
#include <KSystemTimeZone>

using namespace KPIM;
using namespace KCal;

class KPIM::KTimeZoneComboBox::Private
{
  public:
    Private( Calendar *calendar, KTimeZoneComboBox *parent )
      : mParent( parent ), mCalendar( calendar )
    {}

    void fillComboBox() const;
    KTimeZoneComboBox *mParent;
    Calendar *mCalendar;
};

void KTimeZoneComboBox::Private::fillComboBox() const
{
  // Read all system time zones
  QStringList list;

  const KTimeZones::ZoneMap timezones = KSystemTimeZones::zones();
  for ( KTimeZones::ZoneMap::ConstIterator it=timezones.begin(); it != timezones.end(); ++it ) {
    list.append( it.key().toUtf8() );
  }
  list.sort();

  // Prepend the list of timezones from the Calendar
  if ( mCalendar ) {
    const ICalTimeZones::ZoneMap calzones = mCalendar->timeZones()->zones();
    for ( ICalTimeZones::ZoneMap::ConstIterator it=calzones.begin(); it != calzones.end(); ++it ) {
      kDebug() << "Prepend timezone " << it.key().toUtf8();
      list.prepend( it.key().toUtf8() );
    }
  }

  // Prepend UTC and Floating, for convenience
  list.prepend( "UTC" );
  list.prepend( i18n( "Floating" ) );

  mParent->addItems( list );
}

KTimeZoneComboBox::KTimeZoneComboBox( Calendar *calendar, QWidget *parent )
  : KComboBox( parent ), d( new KPIM::KTimeZoneComboBox::Private( calendar, this ) )
{
  KGlobal::locale()->insertCatalog( "timezones4" ); // for translated timezones
  d->fillComboBox();
}

KTimeZoneComboBox::~KTimeZoneComboBox()
{
  delete d;
}

void KTimeZoneComboBox::selectTimeSpec( const KDateTime::Spec &spec )
{
  int nCurrentlySet = -1;

  for ( int i = 0; i < count(); ++i ) {
    if ( itemText( i ) ==  spec.timeZone().name() ) {
      nCurrentlySet = i;
      break;
    }
  }
  if ( nCurrentlySet == -1 ) {
    //TODO: now search in calendar defined timezone list
    if ( spec.isUtc() ) {
      setCurrentIndex( 1 ); // UTC
    } else {
      setCurrentIndex( 0 ); // Floating event
    }
  } else {
    setCurrentIndex( nCurrentlySet );
  }
}

KDateTime::Spec KTimeZoneComboBox::selectedTimeSpec()
{
  KDateTime::Spec spec;

  if ( currentText() == "UTC" ) {
    spec.setType( KDateTime::UTC );
  } else if ( currentText() == i18n( "Floating" ) ) {
    spec = KDateTime::Spec( KDateTime::ClockTime );
  } else {
    //TODO: calendar defined timezone might be selected
    spec.setType( KSystemTimeZones::zone( currentText() ) );
  }

  return spec;
}

void KTimeZoneComboBox::selectLocalTimeSpec()
{
  selectTimeSpec( KDateTime::Spec( KSystemTimeZones::local() ) );
}

void KTimeZoneComboBox::setFloating( bool floating, const KDateTime::Spec &spec )
{
  if ( floating ) {
    selectTimeSpec( KDateTime::ClockTime );
  } else {
    if ( spec.isValid() ) {
      selectTimeSpec( spec );
    } else {
      selectLocalTimeSpec();
    }
  }
}
