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

#include <kglobalsettings.h>

#include <libkdepim/htmldiffalgodisplay.h>

using namespace KPIM;

HTMLDiffAlgoDisplay::HTMLDiffAlgoDisplay( QWidget *parent )
  : KTextBrowser( parent )
{
  setWrapPolicy( QTextEdit::AtWordBoundary );
  setVScrollBarMode( QScrollView::AlwaysOff );
  setHScrollBarMode( QScrollView::AlwaysOff );
}

void HTMLDiffAlgoDisplay::begin()
{
  clear();
  mText = "";

  mText.append( "<html>" );
  mText.append( QString( "<body text=\"%1\" bgcolor=\"%2\">" )
               .arg( KGlobalSettings::textColor().name() )
               .arg( KGlobalSettings::baseColor().name() ) );

  mText.append( "<center><table>" );
  mText.append( QString( "<tr><th></th><th align=\"center\">%1</th><td>         </td><th align=\"center\">%2</th></tr>" )
               .arg( mLeftTitle )
               .arg( mRightTitle ) );
}

void HTMLDiffAlgoDisplay::end()
{
  mText.append( "</table></center>"
                "</body>"
                "</html>" );

  setText( mText );
}

void HTMLDiffAlgoDisplay::setLeftSourceTitle( const QString &title )
{
  mLeftTitle = title;
}

void HTMLDiffAlgoDisplay::setRightSourceTitle( const QString &title )
{
  mRightTitle = title;
}

void HTMLDiffAlgoDisplay::additionalLeftField( const QString &id, const QString &value )
{
  mText.append( QString( "<tr><td align=\"right\"><b>%1:</b></td><td bgcolor=\"#9cff83\">%2</td><td></td><td></td></tr>" )
               .arg( id )
               .arg( value ) );
}

void HTMLDiffAlgoDisplay::additionalRightField( const QString &id, const QString &value )
{
  mText.append( QString( "<tr><td align=\"right\"><b>%1:</b></td><td></td><td></td><td bgcolor=\"#9cff83\">%2</td></tr>" )
               .arg( id )
               .arg( value ) );
}

void HTMLDiffAlgoDisplay::conflictField( const QString &id, const QString &leftValue,
                                          const QString &rightValue )
{
  mText.append( QString( "<tr><td align=\"right\"><b>%1:</b></td><td bgcolor=\"#ff8686\">%2</td><td></td><td bgcolor=\"#ff8686\">%3</td></tr>" )
               .arg( id )
               .arg( leftValue )
               .arg( rightValue ) );
}
