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
#ifndef KDATEPICKERPOPUP_H
#define KDATEPICKERPOPUP_H

#include <qdatetime.h>
#include <qpopupmenu.h>

#include <kdatepicker.h>

class KDatePicker;

/**
   A popup-menu which contains a KDatePicker.

   Actually, this is just a popup-menu with only one widget. The user can choose
   a date like a normal KDatePicker.

   @author Bram Schoenmakers <bram_s@softhome.net>
*/
class KDatePickerPopup: public QPopupMenu
{
  Q_OBJECT
public:
  /**
     A constructor for the KDatePickerPopup.

     @param parent The object's parent.
     @param name The object's name.
  */
  KDatePickerPopup ( QWidget *parent=0, const char *name=0 );

  /**
     A constructor for the KDatePickerPopup

     @param date The initial date.
     @param parent The object's parent.
     @param name The object's name.
  */
  KDatePickerPopup( const QDate & date, QWidget *parent=0,
                    const char *name = 0 );

  virtual ~KDatePickerPopup();

  /**
     @return A pointer to the private variable mDatePicker, an instance of
     KDatePicker.
  */
  KDatePicker *datePicker() const;

signals:

  /**
     This signal gets emitted when the user chooses a date from the
     KDatePicker widget.
  */
  void dateChanged ( QDate );

protected slots:
  void slotDateChanged ( QDate );

private:
  void init();

  KDatePicker *mDatePicker;
};

#endif
