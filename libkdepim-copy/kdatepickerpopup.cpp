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

#include <qapplication.h>
#include <qdatetime.h>
#include <qpopupmenu.h>

#include <kdatepicker.h>

#include "kdatepickerpopup.h"

KDatePickerPopup::KDatePickerPopup(QWidget *parent, const char *name)
  : QPopupMenu( parent, name )
{
  init();
}

KDatePickerPopup::KDatePickerPopup(const QDate &date, QWidget *parent, const char *name)
  : QPopupMenu( parent, name )
{
  init();
  mdatepicker->setDate( date );
}

void KDatePickerPopup::init()
{
  mdatepicker = new KDatePicker( this, QDate::currentDate() ,0 );
  mdatepicker->setCloseButton( false );
  
  insertItem(mdatepicker,0,0);
  
  connect(mdatepicker, SIGNAL( dateEntered( QDate ) ),
          SLOT( slotDateChanged( QDate ) ) );
  connect(mdatepicker, SIGNAL( dateSelected( QDate ) ),
          SLOT( slotDateChanged( QDate ) ) );
}

KDatePickerPopup::~KDatePickerPopup()
{
  delete mdatepicker;
}

KDatePicker *KDatePickerPopup::DatePicker()
{
  return mdatepicker;
}

void KDatePickerPopup::slotDateChanged( QDate date )
{
  emit dateChanged( date );
  hide();
}

#include "kdatepickerpopup.moc"
