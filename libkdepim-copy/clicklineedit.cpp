/*
    This file is part of libkdepim.
    Copyright (c) 2004 Daniel Molkentin <molkentin@kde.org>
    based on code by Cornelius Schumacher <schumacher@kde.org> 

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


#include "clicklineedit.h"
#include <assert.h>


using namespace KPIM;

ClickLineEdit::ClickLineEdit(QWidget *parent, const QString &msg, const char* name) :
  KLineEdit(parent, name), mClickMessage(msg)
{
  setClickMessage( msg );
}

ClickLineEdit::~ClickLineEdit() {}

void ClickLineEdit::setClickMessage( const QString &msg)

{
  if ( msg.isEmpty() ) return;

  mClickMessage = msg;
  setText( msg );
  mFGColor = paletteForegroundColor();
  setPaletteForegroundColor( gray );
}

void ClickLineEdit::focusInEvent(QFocusEvent *ev)
{
  if ( text() == mClickMessage ) setText( QString::null );
  setPaletteForegroundColor( mFGColor );
  QLineEdit::focusInEvent( ev );
}

void ClickLineEdit::focusOutEvent(QFocusEvent *ev)
{
  if ( text().isEmpty() ) setText( mClickMessage );
  setPaletteForegroundColor( gray );
  QLineEdit::focusOutEvent( ev );
}

#include "clicklineedit.moc"
