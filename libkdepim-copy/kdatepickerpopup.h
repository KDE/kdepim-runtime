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

class KDatePickerPopup: public QPopupMenu
{
    Q_OBJECT
  public:
    KDatePickerPopup ( QWidget *parent=0, const char *name=0 );
    KDatePickerPopup( const QDate & date, QWidget *parent=0, const char *name = 0 );
    virtual ~KDatePickerPopup();
    
    void init();
     
    KDatePicker *DatePicker();
                 
  signals:
    void dateChanged ( QDate );
        
  protected slots:
    void slotDateChanged ( QDate );
    
  private:
     KDatePicker *mdatepicker;
};

#endif
