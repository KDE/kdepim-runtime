/****************************************************************************** * *
 *
 *  File : coolcombobox.cpp
 *  Created on Fri 15 May 2009 04:53:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Filtering Framework
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the editoried warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "coolcombobox.h"

#include <QPaintEvent>
#include <QStyleOption>
#include <QStylePainter>
#include <QStyle>


namespace Akonadi
{
namespace Filter
{
namespace UI
{
namespace Private
{

CoolComboBox::CoolComboBox( bool rw, QWidget * parent )
  : KComboBox( rw, parent ), mOpacity( 1.0 ), mOverlayOpacity( 0.1 )
{
}

void CoolComboBox::setOpacity( qreal opacity )
{
  mOpacity = opacity;
  if( opacity > 1.0 )
    opacity = 1.0;
  else if( opacity < 0.0 )
    opacity = 0.0;

  update();
}

void CoolComboBox::setOverlayColor( const QColor &clr )
{
  mOverlayColor = clr;
  update();
}


void CoolComboBox::paintEvent( QPaintEvent * e )
{
  QStylePainter painter( this );
  painter.setPen( palette().color( QPalette::Text ) );

  if( mOpacity < 1.0 )
    painter.setOpacity( mOpacity );

  // draw the combobox frame, focusrect and selected etc.
  QStyleOptionComboBox opt;
  initStyleOption( &opt );
  painter.drawComplexControl( QStyle::CC_ComboBox, opt );

  // draw the icon and text
  painter.drawControl( QStyle::CE_ComboBoxLabel, opt );

  if( mOverlayColor.isValid() )
  {
    painter.setOpacity( mOverlayOpacity * mOpacity );
    painter.fillRect( rect(), mOverlayColor );
  }
}

} // namespace Private

} // namespace UI

} // namespace Filter

} // namespace Akonadi
