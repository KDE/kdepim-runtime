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

#include <qevent.h>
#include <qlineedit.h>
#include <qapplication.h>
#include <qlistbox.h>

#include <kdatepicker.h>
#include <knotifyclient.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>

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
  setEditText(today);
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

void KDateEdit::setDate(const QDate& newDate)
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
  setEditText(dateString);
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
  if (readDate(dt) && (mHandleInvalid || dt.isValid())) {
    return dt;
  }
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

  QDate date;
  readDate(date);
  if (date.isValid()) {
    mDatePicker->setDate( date );
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
