/****************************************************************************** * *
 *
 *  File : expandingscrollarea.h
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

#ifndef _AKONADI_FILTER_UI_PRIVATE_EXPANDINGSCROLLAREA_H_
#define _AKONADI_FILTER_UI_PRIVATE_EXPANDINGSCROLLAREA_H_

#include <akonadi/filter/ui/config-akonadi-filter-ui.h>

#include <QtGui/QScrollArea>

namespace Akonadi
{
namespace Filter
{
namespace UI
{

class ExpandingScrollArea : public QScrollArea
{
  Q_OBJECT
public:
  ExpandingScrollArea( QWidget * parent );
protected:
  bool mAutoExpand;
public:
  bool autoExpand() const
  {
    return mAutoExpand;
  }

  void setAutoExpand( bool b )
  {
    mAutoExpand = b;
  }
protected:
  virtual QSize sizeHint() const;
  virtual QSize minimumSizeHint() const;
  virtual bool eventFilter( QObject *o, QEvent *e );
}; // class ExpandingScrollArea

} // namespace UI

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_UI_PRIVATE_EXPANDINGSCROLLAREA_H_
