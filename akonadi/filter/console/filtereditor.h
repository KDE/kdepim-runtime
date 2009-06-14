/******************************************************************************
 *
 *  File : filtereditor.h
 *  Created on Sat 13 Jun 2009 06:08:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Filter Console Application
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#ifndef _FILTEREDITOR_H_
#define _FILTEREDITOR_H_

#include <KDialog>

class Filter;
class QLineEdit;
class QTreeWidget;
class QPushButton;

namespace Akonadi
{
namespace Filter
{
namespace UI
{
  class ProgramEditor;
} // namespace UI

} // namespace Filter

} // namespace Akonadi


class FilterEditor : public KDialog
{
  Q_OBJECT
public:
  FilterEditor( QWidget * parent, Filter * filter );
  virtual ~FilterEditor();

protected:
  Filter * mFilter;
  QLineEdit * mIdLineEdit;
  QTreeWidget * mCollectionList;
  QPushButton * mAddCollectionButton;
  QPushButton * mRemoveCollectionButton;
  Akonadi::Filter::UI::ProgramEditor * mProgramEditor;
protected slots:
  void slotAddCollectionButtonClicked();
  void slotRemoveCollectionButtonClicked();
};

#endif //!_FILTEREDITOR_H_
