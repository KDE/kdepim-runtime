/*
    This file is part of libkdepim.

    Copyright (c) 2002 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include <qapplication.h>
#include <qevent.h>
#include <qlineedit.h>

#include <kdatepicker.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <knotifyclient.h>

#include "kdateedit.moc"


KDateEdit::KDateEdit(QWidget *parent, const char *name)
  : QComboBox(true, parent, name)
{
  setMaxCount(1);       // need at least one entry for popup to work
  QString today = KGlobal::locale()->formatDate(QDate::currentDate(), true);
  insertItem(today);
  setCurrentItem(0);
  setCurrentText(today);
  setMinimumSize(sizeHint());

  mDateFrame = new QVBox(0,0,WType_Popup);
  mDateFrame->setFrameStyle(QFrame::PopupPanel | QFrame::Raised);
  mDateFrame->setLineWidth(3);
  mDateFrame->hide();

  mDatePicker = new KDatePicker(mDateFrame,QDate::currentDate());

  connect(lineEdit(),SIGNAL(returnPressed()),SLOT(lineEnterPressed()));
  connect(this,SIGNAL(textChanged(const QString &)),
          SLOT(slotTextChanged(const QString &)));

  connect(mDatePicker,SIGNAL(dateEntered(QDate)),SIGNAL(dateEntered(QDate)));
  connect(mDatePicker,SIGNAL(dateSelected(QDate)),SIGNAL(dateSelected(QDate)));

  // Create the keyword list. This will be used to match against when the user
  // enters information.
  mKeywordMap[i18n("tomorrow")] = 1;
  mKeywordMap[i18n("today")] = 0;
  mKeywordMap[i18n("yesterday")] = -1;

  QString dayName;
  for (int i = 1; i <= 7; ++i)
  {
    dayName = KGlobal::locale()->weekDayName(i).lower();
    mKeywordMap[dayName] = i + 100;
  }
  lineEdit()->installEventFilter(this);   // handle keyword entry

  mTextChanged = false;
  mHandleInvalid = false;
}

KDateEdit::~KDateEdit()
{
  delete mDateFrame;
}

void KDateEdit::setDate(QDate newDate)
{
  if (!newDate.isValid() && !mHandleInvalid)
    return;

  QString dateString = "";
  if(newDate.isValid())
    dateString = KGlobal::locale()->formatDate( newDate, true );

  mTextChanged = false;

  // We do not want to generate a signal here, since we explicity setting
  // the date
  bool b = signalsBlocked();
  blockSignals(true);
  setCurrentText(dateString);
  blockSignals(b);
}

void KDateEdit::setHandleInvalid(bool handleInvalid)
{
  mHandleInvalid = handleInvalid;
}

QDate KDateEdit::date() const
{
  QDate date = readDate();

  if (date.isValid() || mHandleInvalid) {
    return date;
  } else {
    KNotifyClient::beep();
    return QDate::currentDate();
  }
}

void KDateEdit::popup()
{
  QPoint popupPoint = mapToGlobal( QPoint( 0,0 ) );
  if ( popupPoint.x() < 0 ) popupPoint.setX( 0 );

  int desktopHeight = QApplication::desktop()->height();
  int dateFrameHeight = mDateFrame->sizeHint().height();

  if ( popupPoint.y() + height() + dateFrameHeight > desktopHeight ) {
    popupPoint.setY( popupPoint.y() - dateFrameHeight );
  } else {
    popupPoint.setY( popupPoint.y() + height() );
  }

  mDateFrame->move( popupPoint );

  QDate date = readDate();
  if(date.isValid()) {
    mDatePicker->setDate( date );
  } else {
    mDatePicker->setDate( QDate::currentDate() );
  }

  mDateFrame->show();
}

void KDateEdit::dateSelected(QDate newDate)
{
  if (newDate.isValid() || mHandleInvalid)
  {
    setDate(newDate);
    emit dateChanged(newDate);
    mDateFrame->hide();
  }
}

void KDateEdit::dateEntered(QDate newDate)
{
  if (newDate.isValid() || mHandleInvalid)
  {
    setDate(newDate);
    emit dateChanged(newDate);
  }
}

void KDateEdit::lineEnterPressed()
{
  QDate date = readDate();

  if(date.isValid())
  {
    // Update the edit. This is needed if the user has entered a
    // word rather than the actual date.
    setDate(date);

    emit(dateChanged(date));
  }
}

bool KDateEdit::inputIsValid()
{
  return readDate().isValid();
}

QDate KDateEdit::readDate() const
{
  QString text = currentText();
  QDate date;

  if (mKeywordMap.contains(text.lower()))
  {
    int i = mKeywordMap[text.lower()];
    if (i >= 100)
    {
      /* A day name has been entered. Convert to offset from today.
       * This uses some math tricks to figure out the offset in days
       * to the next date the given day of the week occurs. There
       * are two cases, that the new day is >= the current day, which means
       * the new day has not occured yet or that the new day < the current day,
       * which means the new day is already passed (so we need to find the
       * day in the next week).
       */
      i -= 100;
      int currentDay = QDate::currentDate().dayOfWeek();
      if (i >= currentDay)
        i -= currentDay;
      else
        i += 7 - currentDay;
    }
    date = QDate::currentDate().addDays(i);
  }
  else
  {
    date = KGlobal::locale()->readDate(text);
  }

  return date;
}

bool KDateEdit::eventFilter(QObject *, QEvent *e)
{
  // We only process the focus out event if the text has changed
  // since we got focus
  if ((e->type() == QEvent::FocusOut) && mTextChanged)
  {
    lineEnterPressed();
    mTextChanged = false;
  }

  return false;
}

void KDateEdit::slotTextChanged(const QString &)
{
  if(mHandleInvalid && currentText().stripWhiteSpace().isEmpty()) {
    QDate date; //invalid date
    emit(dateChanged(date));
  } else  {
    mTextChanged = true;
  }
}
