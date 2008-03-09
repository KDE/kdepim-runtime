/**
 * kmstylelistselectaction.cpp
 *
 * Copyright (C)  2007 Laurent Montel <montel@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "kmstylelistselectaction.h"
#include <klocale.h>

using namespace KPIM;

KMStyleListSelectAction::KMStyleListSelectAction( const QString& text, QWidget *parent)
 : KSelectAction(text, parent)
{
  init();
  connect( this, SIGNAL( triggered(int) ),
           this, SLOT( slotChangeStyle(int) ) );
}

void KMStyleListSelectAction::setCurrentStyle( QTextListFormat::Style style )
{
  switch ( style ) {
    default:
    case QTextListFormat::ListStyleUndefined:
      setCurrentItem( 0 );
      break;
    case QTextListFormat::ListDisc:
      setCurrentItem( 1 );
      break;
    case QTextListFormat::ListCircle:
      setCurrentItem( 2 );
      break;
    case QTextListFormat::ListSquare:
      setCurrentItem( 3 );
      break;
    case QTextListFormat::ListDecimal:
      setCurrentItem( 4 );
      break;
    case QTextListFormat::ListLowerAlpha:
      setCurrentItem( 5 );
      break;
    case QTextListFormat::ListUpperAlpha:
      setCurrentItem( 6 );
      break;
  }
}

void KMStyleListSelectAction::slotChangeStyle( int styleIndex )
{
  QTextListFormat::Style paragStyle = QTextListFormat::ListStyleUndefined;
  switch ( styleIndex ) {
    default:
    case 0:
      paragStyle = QTextListFormat::ListStyleUndefined;
      break;
    case 1:
      paragStyle = QTextListFormat::ListDisc;
      break;
    case 2:
      paragStyle = QTextListFormat::ListCircle;
      break;
    case 3:
      paragStyle = QTextListFormat::ListSquare;
      break;
    case 4:
      paragStyle = QTextListFormat::ListDecimal;
      break;
    case 5:
      paragStyle = QTextListFormat::ListLowerAlpha;
      break;
    case 6:
      paragStyle = QTextListFormat::ListUpperAlpha;
      break;
  }

  emit applyStyle( paragStyle );
}

void KMStyleListSelectAction::init()
{
  QStringList styleItems;
  styleItems << i18n( "Standard" );
  styleItems << i18n( "Bulleted List (Disc)" );
  styleItems << i18n( "Bulleted List (Circle)" );
  styleItems << i18n( "Bulleted List (Square)" );
  styleItems << i18n( "Ordered List (Decimal)" );
  styleItems << i18n( "Ordered List (Alpha lower)" );
  styleItems << i18n( "Ordered List (Alpha upper)" );
  setItems( styleItems );
  setCurrentItem( 0 );
}

#include "kmstylelistselectaction.moc"
