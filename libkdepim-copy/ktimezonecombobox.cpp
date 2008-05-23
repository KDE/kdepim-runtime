/*
  Copyright (C) 2007  Bruno Virlet <bruno.virlet@gmail.com>

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

#include <KLocale>
#include <KSystemTimeZone>

using namespace KPIM;

class KTimeZoneComboBox::Private
{
public:
  Private( KTimeZoneComboBox *parent )
    : mParent( parent )
    {}

  void fillComboBox() const;

  KTimeZoneComboBox *mParent;
};


void KTimeZoneComboBox::Private::fillComboBox() const
{
  // Read all system time zones
  QStringList list;

  const KTimeZones::ZoneMap timezones = KSystemTimeZones::zones();

  for (KTimeZones::ZoneMap::ConstIterator it = timezones.begin();  it != timezones.end();  ++it) {
    list.append(i18n(it.key().toUtf8()));
  }

  list.sort();
  list.prepend( "UTC" );
  list.prepend( i18n( "Floating" ) );

  mParent->addItems( list );
}


KTimeZoneComboBox::KTimeZoneComboBox( QWidget* parent )
  : KComboBox( parent ),
    d( new Private( this ) )
{
  d->fillComboBox();
}

KTimeZoneComboBox::~KTimeZoneComboBox()
{
  delete d;
}

void KTimeZoneComboBox::selectTimeSpec( const KDateTime::Spec &spec )
{
  int nCurrentlySet = -1;

  for ( int i = 0; i < count(); ++i )
  {
    if ( itemText(i) ==  spec.timeZone().name() )
    {
      nCurrentlySet = i;
      break;
    }
  }
  if ( nCurrentlySet == -1 ) {
    if ( spec.isUtc() )
      setCurrentIndex( 1 ); // UTC
    else
      setCurrentIndex( 0 ); // Floating event
  }
  else
    setCurrentIndex( nCurrentlySet );
}

void KTimeZoneComboBox::selectLocalTimeSpec()
{
  selectTimeSpec( KDateTime::Spec( KSystemTimeZones::local() ) );
}

KDateTime::Spec KTimeZoneComboBox::selectedTimeSpec()
{
  KDateTime::Spec spec;

  if ( currentText() == "UTC" )
    spec.setType( KDateTime::UTC );
  else if ( currentText() == i18n( "Floating" ) )
    spec = KDateTime::Spec( KDateTime::ClockTime );
  else
    spec.setType( KSystemTimeZones::zone( currentText() ) );

  return spec;
}

void KTimeZoneComboBox::setFloating( bool floating, const KDateTime::Spec &spec )
{
  if ( floating )
    selectTimeSpec( KDateTime::ClockTime );
  else {
    if ( spec.isValid() )
      selectTimeSpec( spec );
    else
      selectLocalTimeSpec();
  }
}
