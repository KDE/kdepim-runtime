/****************************************************************************** * *
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

class QListWidget;
class QPushButton;

class MainWindow : public KXmlGuiWindow
{
  Q_OBJECT
public:
  MainWindow();
  virtual ~MainWindow();
public:
  QListWidget * mFilterListWidget;
  QPushButton * mNewFilterButton;
  QPushButton * mDeleteFilterButton;

protected:
  void listFilters();

protected slots:
  void slotNewFilterButtonClicked();
  void slotDeleteFilterButtonClicked();
};

#endif //!_MAINWINDOW_H_
