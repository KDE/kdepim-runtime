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

#include <qdatetime.h>
#include <q3popupmenu.h>

#include <klocale.h>

#include "kdatepickerpopup.h"

KDatePickerPopup::KDatePickerPopup( int items, const QDate &date, QWidget *parent,
                                    const char *name )
  : Q3PopupMenu( parent, name )
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
    #warning Port me!
    //insertItem( mDatePicker );

    if ( ( mItems & NoDate ) || ( mItems & Words ) )
      insertSeparator();
  }

  if ( mItems & Words ) {
    insertItem( i18n("&Today"), this, SLOT( slotToday() ) );
    insertItem( i18n("To&morrow"), this, SLOT( slotTomorrow() ) );
    insertItem( i18n("&Friday"), this, SLOT( slotFriday() ) );
    insertItem( i18n("&Sunday"), this, SLOT( slotSunday() ) );
    insertItem( i18n("Next &Week"), this, SLOT( slotNextWeek() ) );
    insertItem( i18n("Next M&onth"), this, SLOT( slotNextMonth() ) );

    if ( mItems & NoDate )
      insertSeparator();
  }

  if ( mItems & NoDate )
    insertItem( i18n("No Date"), this, SLOT( slotNoDate() ) );
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

void KDatePickerPopup::slotFriday()
{
  QDate date = QDate::currentDate();
  int day = date.dayOfWeek();
  if ( day < 6 )
    date = date.addDays( 5 - day );
  else
    date = date.addDays( 5 - day + 7 );

  emit dateChanged( date );
}

void KDatePickerPopup::slotSunday()
{
  QDate date = QDate::currentDate();
  emit dateChanged( date.addDays( 7 - date.dayOfWeek() ) );
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
