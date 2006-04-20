/* This file has been copied from the KDE libraries and slightly modified.
   This can be removed as soon as kdelibs provides the same functionality.

   Copyright (C) 2000 Ronny Standtke <Ronny.Standtke@gmx.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "krsqueezedtextlabel.h"
#include "kstringhandler.h"
#include <qtooltip.h>
//Added by qt3to4:
#include <QLabel>
#include <QResizeEvent>

KRSqueezedTextLabel::KRSqueezedTextLabel( const QString &text , QWidget *parent, const char *name )
 : QLabel ( parent ) {
  setObjectName( name );
  setSizePolicy(QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
  fullText = text;
  squeezeTextToLabel();
}

KRSqueezedTextLabel::KRSqueezedTextLabel( QWidget *parent, const char *name )
 : QLabel ( parent ) {
  setObjectName( name );
  setSizePolicy(QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
}

void KRSqueezedTextLabel::resizeEvent( QResizeEvent * ) {
  squeezeTextToLabel();
}

QSize KRSqueezedTextLabel::minimumSizeHint() const
{
  QSize sh = QLabel::minimumSizeHint();
  sh.setWidth(-1);
  return sh;
}

QSize KRSqueezedTextLabel::sizeHint() const
{
  return QSize(contentsRect().width(), QLabel::sizeHint().height());
}

void KRSqueezedTextLabel::setText( const QString &text ) {
  fullText = text;
  squeezeTextToLabel();
}

void KRSqueezedTextLabel::squeezeTextToLabel() {
  QFontMetrics fm(fontMetrics());
  int labelWidth = size().width();
  int textWidth = fm.width(fullText);
  if (textWidth > labelWidth) {
    QString squeezedText = KStringHandler::rPixelSqueeze(fullText, fm, labelWidth);
    QLabel::setText(squeezedText);

    this->setToolTip( fullText );

  } else {
    QLabel::setText(fullText);
	this->setToolTip("");

  }
}

void KRSqueezedTextLabel::setAlignment( int alignment )
{
  // save fullText and restore it
  QString tmpFull(fullText);
  QLabel::setAlignment(alignment);
  fullText = tmpFull;
}

#include "krsqueezedtextlabel.moc"
