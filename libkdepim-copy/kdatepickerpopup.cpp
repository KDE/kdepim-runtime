/*
  This file is part of libkdepim.

  Copyright (c) 2004 Bram Schoenmakers <bram_s@softhome.net>

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
  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA.
*/

#include <qdatetime.h>
#include <qpopupmenu.h>

#include <klocale.h>
#include <kdatepicker.h>

#include "kdatepickerpopup.h"

KDatePickerPopup::KDatePickerPopup( int items, const QDate &date, QWidget *parent,
                                    const char *name )
  : QPopupMenu( parent, name )
{
  mItems = items;

  mDatePicker = new KDatePicker( this, QDate::currentDate(), 0 );
  mDatePicker->setCloseButton( false );

  connect( mDatePicker, SIGNAL( dateEntered( QDate ) ),
           SLOT( slotDateChanged( QDate ) ) );
  connect( mDatePicker, SIGNAL( dateSelected( QDate ) ),
           SLOT( slotDateChanged( QDate ) ) );

  mDatePicker->setDate( date );

  buildMenu();
}

void KDatePickerPopup::buildMenu()
{
  clear();

  if ( ( mItems & DatePicker ) == DatePicker ) {
    insertItem( mDatePicker );

  if ( ( mItems & NoDate ) == NoDate || ( mItems & Words ) == Words )
    insertSeparator();
  }

  if ( ( mItems & Words ) == Words ) {
    insertItem( i18n("Today"), this, SLOT( slotToday() ) );
    insertItem( i18n("Tomorrow"), this, SLOT( slotTomorrow() ) );
    insertItem( i18n("Friday"), this, SLOT( slotFriday() ) );
    insertItem( i18n("Sunday"), this, SLOT( slotSunday() ) );
    insertItem( i18n("Next Week"), this, SLOT( slotNextWeek() ) );
    insertItem( i18n("Next Month"), this, SLOT( slotNextMonth() ) );

    if ( ( mItems & NoDate ) == NoDate )
      insertSeparator();
  }

  if ( ( mItems & NoDate ) == NoDate ) {
    insertItem( i18n("No Date"), this, SLOT( slotNoDate() ) );
  }
}

KDatePickerPopup::~KDatePickerPopup()
{
  delete mDatePicker;
}

KDatePicker *KDatePickerPopup::datePicker() const
{
  return mDatePicker;
}

void KDatePickerPopup::setItems( int items )
{
  mItems = items;
  buildMenu();
}

void KDatePickerPopup::slotDateChanged( QDate date )
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
