/*
    This file is part of libkdepim.

    Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

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

#ifndef KPIM_HTMLDIFFALGODISPLAY_H
#define KPIM_HTMLDIFFALGODISPLAY_H

#include <ktextbrowser.h>
#include <libkdepim/diffalgo.h>

namespace KPIM {

class KDE_EXPORT HTMLDiffAlgoDisplay : virtual public DiffAlgoDisplay, public KTextBrowser
{
  public:
    HTMLDiffAlgoDisplay( QWidget *parent );

    void begin();
    void end();
    void setLeftSourceTitle( const QString &title );
    void setRightSourceTitle( const QString &title );
    void additionalLeftField( const QString &id, const QString &value );
    void additionalRightField( const QString &id, const QString &value );
    void conflictField( const QString &id, const QString &leftValue,
                        const QString &rightValue );

  private:
    QString mLeftTitle;
    QString mRightTitle;
    QString mText;
};

}

#endif
