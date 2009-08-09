/****************************************************************************** * *
 *
 *  File : widgethighlighter.h
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

#ifndef _AKONADI_FILTER_UI_PRIVATE_WIDGETHIGHLIGHTER_H_
#define _AKONADI_FILTER_UI_PRIVATE_WIDGETHIGHLIGHTER_H_

#include <akonadi/filter/ui/config-akonadi-filter-ui.h>

#include <QtGui/QWidget>
#include <QtGui/QPalette>

class QTimer;

namespace Akonadi
{
namespace Filter
{
namespace UI
{
namespace Private
{

class WidgetHighlighter : public QObject
{
  Q_OBJECT
public:
  WidgetHighlighter( QWidget * targetAndParent );
  ~WidgetHighlighter();

private:
  QPalette mSavedPalette;
  bool mSavedAutoFillBackground;
  bool mHighlightingOn;
  QTimer * mSwitchOffTimer;
  QTimer * mFlashTimer;
  bool mFlashOn;

public:
  bool eventFilter( QObject * o, QEvent * e );

private:
  void hookEventFilter( QWidget * target );
  void unhookEventFilter( QWidget * target );

  void triggerSwitchHighlightingOff();
  void switchHighlightingOn();
  void switchHighlightingOff();

  void highlight( QWidget * target, int percent );

private slots:
  void slotFlashWidget();
  void slotSwitchHighlightingOff();

}; // class CoolComboBox

} // namespace Private

} // namespace UI

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_UI_PRIVATE_WIDGETHIGHLIGHTER_H_
