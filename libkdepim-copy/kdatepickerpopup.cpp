/*
  This file is part of libkdepim.

  Copyright (c) 2004 Bram Schoenmakers <bramschoenmakers@kde.nl>

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

#include <QDateTime>

#include <klocale.h>

#include "kdatepickerpopup.h"

KDatePickerPopup::KDatePickerPopup( int items, const QDate &date, QWidget *parent )
  : QMenu( parent )
{
  mItems = items;

  mDatePicker = new KDatePicker( this );
  mDatePicker->setCloseButton( false );

  connect( mDatePicker, SIGNAL( dateEntered( const QDate& ) ),
           SLOT( slotDateChanged( const QDate& ) ) );
  connect( mDatePicker, SIGNAL( dateSelected( const QDate& ) ),
           SLOT( slotDateChanged( const QDate& ) ) );

  mDatePicker->setDate( date );

  buildMenu();
}

void KDatePickerPopup::buildMenu()
{
  if ( isVisible() ) return;
  clear();

  if ( mItems & DatePicker ) {
#ifdef __GNUC__
    #warning Port me!
#endif
    //addAction( mDatePicker );

    if ( ( mItems & NoDate ) || ( mItems & Words ) )
      addSeparator();
  }

  if ( mItems & Words ) {
    addAction( i18n("&Today"), this, SLOT( slotToday() ) );
    addAction( i18n("To&morrow"), this, SLOT( slotTomorrow() ) );
    addAction( i18n("Next &Week"), this, SLOT( slotNextWeek() ) );
    addAction( i18n("Next M&onth"), this, SLOT( slotNextMonth() ) );

    if ( mItems & NoDate )
      addSeparator();
  }

  if ( mItems & NoDate )
    addAction( i18n("No Date"), this, SLOT( slotNoDate() ) );
}

KDatePicker *KDatePickerPopup::datePicker() const
{
  return mDatePicker;
}

void KDatePickerPopup::setDate( const QDate &date )
{
  mDatePicker->setDate( date );
}

#if 0
void KDatePickerPopup::setItems( int items )
{
  mItems = items;
  buildMenu();
}
#endif

void KDatePickerPopup::slotDateChanged( const QDate &date )
{
  emit dateChanged( date );
  hide();
}

void KDatePickerPopup::slotToday()
{
  emit dateChanged( QDate::currentDate() );
}

void KDatePickerPopup::slotTomorrow()
{
  emit dateChanged( QDate::currentDate().addDays( 1 ) );
}

void KDatePickerPopup::slotNoDate()
{
  emit dateChanged( QDate() );
}

void KDatePickerPopup::slotNextWeek()
{
  emit dateChanged( QDate::currentDate().addDays( 7 ) );
}

void KDatePickerPopup::slotNextMonth()
{
  emit dateChanged( QDate::currentDate().addMonths( 1 ) );
}

#include "kdatepickerpopup.moc"
