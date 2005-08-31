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

#ifndef KRSQUEEZEDTEXTLABEL_H
#define KRSQUEEZEDTEXTLABEL_H

#include <qlabel.h>
#include <kdepimmacros.h>

/**
 * @short A replacement for QLabel that squeezes its text
 *
 * A label class that squeezes its text into the label
 *
 * If the text is too long to fit into the label it is divided into
 * remaining left and right parts which are separated by three dots.
 *
 * @author Ronny Standtke <Ronny.Standtke@gmx.de>
 */

/*
 * QLabel
 */
class KDE_EXPORT KRSqueezedTextLabel : public QLabel {
  Q_OBJECT

public:
  /**
   * Default constructor.
   */
  KRSqueezedTextLabel( QWidget *parent, const char *name = 0 );
  KRSqueezedTextLabel( const QString &text, QWidget *parent, const char *name = 0 );

  virtual QSize minimumSizeHint() const;
  virtual QSize sizeHint() const;
  /**
   * Overridden for internal reasons; the API remains unaffected.
   */
  virtual void setAlignment( int );

public slots:
  void setText( const QString & );

protected:
  /**
   * used when widget is resized
   */
  void resizeEvent( QResizeEvent * );
  /**
   * does the dirty work
   */
  void squeezeTextToLabel();
  QString fullText;

};

#endif // KRSQUEEZEDTEXTLABEL_H
