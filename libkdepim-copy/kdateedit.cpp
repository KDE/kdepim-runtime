/*
    This file is part of libkdepim.

    Copyright (c) 2002 Cornelius Schumacher <schumacher@kde.org>
    Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

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

#include <qevent.h>
#include <qlineedit.h>
#include <qapplication.h>
#include <qlistbox.h>

#include <kdatepicker.h>
#include <knotifyclient.h>
#include <kglobalsettings.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kcalendarsystem.h>

#include "kdateedit.h"
#include "kdateedit.moc"


KDateEdit::KDateEdit(QWidget *parent, const char *name)
  : QComboBox(true, parent, name),
    defaultValue(QDate::currentDate()),
    mReadOnly(false),
    mDiscardNextMousePress(false)
{
  setMaxCount(1);       // need at least one entry for popup to work
  value = defaultValue;
  QString today = KGlobal::locale()->formatDate(value, true);
  insertItem(today);
  setCurrentItem(0);
  changeItem(today, 0);
  setMinimumSize(sizeHint());

  mDateFrame = new QVBox(0,0,WType_Popup);
  mDateFrame->setFrameStyle(QFrame::PopupPanel | QFrame::Raised);
  mDateFrame->setLineWidth(3);
  mDateFrame->hide();
  mDateFrame->installEventFilter(this);

  mDatePicker = new KDatePicker(mDateFrame, value);

  connect(lineEdit(),SIGNAL(returnPressed()),SLOT(lineEnterPressed()));
  connect(this,SIGNAL(textChanged(const QString &)),
          SLOT(slotTextChanged(const QString &)));

  connect(mDatePicker,SIGNAL(dateEntered(QDate)),SLOT(dateEntered(QDate)));
  connect(mDatePicker,SIGNAL(dateSelected(QDate)),SLOT(dateSelected(QDate)));

  // Create the keyword list. This will be used to match against when the user
  // enters information.
  mKeywordMap[i18n("tomorrow")] = 1;
  mKeywordMap[i18n("today")] = 0;
  mKeywordMap[i18n("yesterday")] = -1;

  QString dayName;
  for (int i = 1; i <= 7; ++i)
  {
    dayName = KGlobal::locale()->calendar()->weekDayName(i).lower();
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

void KDateEdit::setDate(const QDate& newDate)
{
  if (!newDate.isValid() && !mHandleInvalid)
    return;

  QString dateString = "";
  if(newDate.isValid())
    dateString = KGlobal::locale()->formatDate( newDate, true );

  mTextChanged = false;

  // We do not want to generate a signal here, since we explicitly setting
  // the date
  bool b = signalsBlocked();
  blockSignals(true);
  changeItem(dateString, 0);
  blockSignals(b);

  value = newDate;
}

void KDateEdit::setHandleInvalid(bool handleInvalid)
{
  mHandleInvalid = handleInvalid;
}

bool KDateEdit::handlesInvalid() const
{
  return mHandleInvalid;
}

void KDateEdit::setReadOnly(bool readOnly)
{
  mReadOnly = readOnly;
  lineEdit()->setReadOnly(readOnly);
}

bool KDateEdit::isReadOnly() const
{
  return mReadOnly;
}

bool KDateEdit::validate( const QDate & )
{
  return true;
}

QDate KDateEdit::date() const
{
  QDate dt;
  if ( mHandleInvalid || value.isValid() )
    return value;
  return defaultValue;
}

QDate KDateEdit::defaultDate() const
{
  return defaultValue;
}

void KDateEdit::setDefaultDate(const QDate& date)
{
  defaultValue = date;
}

void KDateEdit::popup()
{
  if (mReadOnly)
    return;

  QRect desk = KGlobalSettings::desktopGeometry(this);

  QPoint popupPoint = mapToGlobal( QPoint( 0,0 ) );

  int dateFrameHeight = mDateFrame->sizeHint().height();
  if ( popupPoint.y() + height() + dateFrameHeight > desk.bottom() ) {
    popupPoint.setY( popupPoint.y() - dateFrameHeight );
  } else {
    popupPoint.setY( popupPoint.y() + height() );
  }
  int dateFrameWidth = mDateFrame->sizeHint().width();
  if ( popupPoint.x() + dateFrameWidth > desk.right() ) {
    popupPoint.setX( desk.right() - dateFrameWidth );
  }

  if ( popupPoint.x() < desk.left() ) popupPoint.setX( desk.left() );
  if ( popupPoint.y() < desk.top() ) popupPoint.setY( desk.top() );

  mDateFrame->move( popupPoint );

  if (value.isValid()) {
    mDatePicker->setDate( value );
  } else {
    mDatePicker->setDate( defaultValue );
  }

  mDateFrame->show();

  // The combo box is now shown pressed. Make it show not pressed again
  // by causing its (invisible) list box to emit a 'selected' signal.
  QListBox *lb = listBox();
  if (lb) {
    lb->setCurrentItem(0);
    QKeyEvent* keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, 0, 0);
    QApplication::postEvent(lb, keyEvent);
  }
}

void KDateEdit::dateSelected(QDate newDate)
{
  if ((mHandleInvalid || newDate.isValid()) && validate(newDate)) {
    setDate(newDate);
    emit dateChanged(newDate);
    mDateFrame->hide();
  }
}

void KDateEdit::dateEntered(QDate newDate)
{
  if ((mHandleInvalid || newDate.isValid()) && validate(newDate)) {
    setDate(newDate);
    emit dateChanged(newDate);
  }
}

void KDateEdit::lineEnterPressed()
{
  QDate date;
  if (readDate(date) && (mHandleInvalid || date.isValid()) && validate(date))
  {
    // Update the edit. This is needed if the user has entered a
    // word rather than the actual date.
    setDate(date);
    emit(dateChanged(date));
  }
  else {
    // Invalid or unacceptable date - revert to previous value
    KNotifyClient::beep();
    setDate(value);
    emit invalidDateEntered();
  }
}

bool KDateEdit::inputIsValid() const
{
  QDate date;
  return readDate(date) && date.isValid();
}

/* Reads the text from the line edit. If the text is a keyword, the
 * word will be translated to a date. If the text is not a keyword, the
 * text will be interpreted as a date.
 * Returns true if the date text is blank or valid, false otherwise.
 */
bool KDateEdit::readDate(QDate& result) const
{
  QString text = currentText();

  if (text.isEmpty()) {
    result = QDate();
  }
  else if (mKeywordMap.contains(text.lower()))
  {
    QDate today = QDate::currentDate();
    int i = mKeywordMap[text.lower()];
    if (i >= 100)
    {
      /* A day name has been entered. Convert to offset from today.
       * This uses some math tricks to figure out the offset in days
       * to the next date the given day of the week occurs. There
       * are two cases, that the new day is >= the current day, which means
       * the new day has not occurred yet or that the new day < the current day,
       * which means the new day is already passed (so we need to find the
       * day in the next week).
       */
      i -= 100;
      int currentDay = today.dayOfWeek();
      if (i >= currentDay)
        i -= currentDay;
      else
        i += 7 - currentDay;
    }
    result = today.addDays(i);
  }
  else
  {
    result = KGlobal::locale()->readDate(text);
    return result.isValid();
  }

  return true;
}

/* Checks for a focus out event. The display of the date is updated
 * to display the proper date when the focus leaves.
 */
bool KDateEdit::eventFilter(QObject *obj, QEvent *e)
{
  if (obj == lineEdit()) {
    // We only process the focus out event if the text has changed
    // since we got focus
    if ((e->type() == QEvent::FocusOut) && mTextChanged)
    {
      lineEnterPressed();
      mTextChanged = false;
    }
    else if (e->type() == QEvent::KeyPress)
    {
      // Up and down arrow keys step the date
      QKeyEvent* ke = (QKeyEvent*)e;

      if (ke->key() == Qt::Key_Return)
      {
        lineEnterPressed();
        return true;
      }

      int step = 0;
      if (ke->key() == Qt::Key_Up)
        step = 1;
      else if (ke->key() == Qt::Key_Down)
        step = -1;
      if (step && !mReadOnly)
      {
        QDate date;
        if (readDate(date) && date.isValid()) {
          date = date.addDays(step);
          if (validate(date)) {
            setDate(date);
            emit(dateChanged(date));
            return true;
          }
        }
      }
    }
  }
  else {
    // It's a date picker event
    switch (e->type()) {
      case QEvent::MouseButtonDblClick:
      case QEvent::MouseButtonPress: {
        QMouseEvent *me = (QMouseEvent*)e;
        if (!mDateFrame->rect().contains(me->pos())) {
          QPoint globalPos = mDateFrame->mapToGlobal(me->pos());
          if (QApplication::widgetAt(globalPos, true) == this) {
            // The date picker is being closed by a click on the
            // KDateEdit widget. Avoid popping it up again immediately.
            mDiscardNextMousePress = true;
          }
        }
        break;
      }
      default:
        break;
    }
  }

  return false;
}

void KDateEdit::mousePressEvent(QMouseEvent *e)
{
  if (e->button() == Qt::LeftButton  &&  mDiscardNextMousePress) {
    mDiscardNextMousePress = false;
    return;
  }
  QComboBox::mousePressEvent(e);
}

void KDateEdit::slotTextChanged(const QString &)
{
  mTextChanged = true;
}
