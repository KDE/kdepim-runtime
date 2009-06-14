/****************************************************************************** * *
 *
 *  File : expandingscrollarea.cpp
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

#include <akonadi/filter/ui/expandingscrollarea.h>

#include <QtCore/QEvent>
#include <QtGui/QLayout>

namespace Akonadi
{
namespace Filter
{
namespace UI
{

ExpandingScrollArea::ExpandingScrollArea( QWidget * parent )
  : QScrollArea( parent ), mAutoExpand( true )
{
}

bool ExpandingScrollArea::eventFilter( QObject *o, QEvent *e )
{
  if( !mAutoExpand )
    return QScrollArea::eventFilter( o, e );

  // QScrollArea monitors the child widget events and updates its scrollbars
  // when its resized.
  // We actually DON'T want the vertical scrollbar to appear in this area
  // so we monitor the event too and actually force the parent
  // to redo a layout (which will grow this QScrollArea size because of
  // the size policies and propagated size hints).

  if( o == widget() && e->type() == QEvent::Resize )
  {
    // This machinery is needed in order to trigger the parent geometry update
    updateGeometry();

    if( parentWidget()->layout() )
    {
      parentWidget()->layout()->invalidate();
      parentWidget()->layout()->update();
    }

    //parentWidget()->updateGeometry();
  }
  return QScrollArea::eventFilter( o, e );
}


QSize ExpandingScrollArea::sizeHint() const
{
  if( !mAutoExpand || !widget() )
    return QScrollArea::sizeHint();
  return widget()->sizeHint();
}

QSize ExpandingScrollArea::minimumSizeHint() const
{
  if( !mAutoExpand || !widget() )
    return QScrollArea::minimumSizeHint();
  return widget()->minimumSizeHint();
}

} // namespace UI

} // namespace Filter

} // namespace Akonadi

