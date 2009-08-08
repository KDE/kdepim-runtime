/******************************************************************************
 *
 *  File : mainwindow.h
 *  Created on Mon 08 Jun 2009 22:38:16 by Szymon Tomasz Stefanek
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

#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <KXmlGuiWindow>

#include <QHash>

class FilterListWidget;
class FilterListWidgetItem;
class QPushButton;

class Filter;

namespace Akonadi {
namespace Filter {
  class ComponentFactory;

  namespace UI {
    class EditorFactory;
  } // namespace UI

} // namespace Filter
} // namespace Akonadi

class OrgFreedesktopAkonadiFilterAgentInterface;


class MainWindow : public KXmlGuiWindow
{
  Q_OBJECT
public:
  MainWindow();
  virtual ~MainWindow();
public:
  FilterListWidget * mFilterListWidget;
  QPushButton * mNewFilterButtonLBB;
  QPushButton * mNewFilterButtonTBB;
  QPushButton * mEditFilterButtonLBB;
  QPushButton * mEditFilterButtonTBB;
  QPushButton * mApplyFilterToItemButton;
  QPushButton * mDeleteFilterButton;
  static MainWindow * mInstance;
  OrgFreedesktopAkonadiFilterAgentInterface * mFilterAgent;
  qlonglong mPendingFilteringJobId;
  /**
   * The hash table of the filter component factories indicized by mimetype.
   */
  QHash< QString, Akonadi::Filter::ComponentFactory * > mComponentFactories;
  QHash< QString, Akonadi::Filter::UI::EditorFactory * > mEditorFactories;

public:
  static MainWindow * instance()
  {
    return mInstance;
  }
 
protected:
  void listFilters();
  void newFilter( bool lbb );
  void editFilter( bool lbb );
  bool fetchFilterData( Filter * filter, QString &source );
  void enableDisableButtons();

protected slots:
  void slotFilteringJobTerminated( qlonglong jobId, int status );
  void slotListFilters();
  void slotNewFilterLBBButtonClicked();
  void slotNewFilterTBBButtonClicked();
  void slotEditFilterLBBButtonClicked();
  void slotEditFilterTBBButtonClicked();
  void slotDeleteFilterButtonClicked();
  void slotApplyFilterToItemButtonClicked();
  void slotFilterListWidgetSelectionChanged();
};

#endif //!_MAINWINDOW_H_
