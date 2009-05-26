/****************************************************************************** * *
 *
 *  File : extensionlabel.cpp
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

#include "extensionlabel.h"

#include <QPaintEvent>
#include <QPainter>


namespace Akonadi
{
namespace Filter
{
namespace UI
{
namespace Private
{

ExtensionLabel::ExtensionLabel( QWidget * parent )
  : QWidget( parent ), mFixedHeight( -1 ), mOpacity( 1.0 )
{
}

void ExtensionLabel::setFixedHeight( int h )
{
  mFixedHeight = h;
  update();
}

void ExtensionLabel::setOpacity( qreal opacity )
{
  mOpacity = opacity;
  if( opacity > 1.0 )
    opacity = 1.0;
  else if( opacity < 0.0 )
    opacity = 0.0;

  update();
}

void ExtensionLabel::setOverlayColor( const QColor &clr )
{
  mOverlayColor = clr;
  update();
}


void ExtensionLabel::paintEvent( QPaintEvent * e )
{
  QWidget::paintEvent( e );

  QPainter p( this );
  p.setOpacity( 0.2 );
  p.setPen( mOverlayColor );

  int h = mFixedHeight >= 0 ? mFixedHeight : ( height() - 5 );

  p.drawLine( 5, 0, 5, h );
  p.drawLine( 5, h, 10, h );
}

} // namespace Private

} // namespace UI

} // namespace Filter

} // namespace Akonadi
