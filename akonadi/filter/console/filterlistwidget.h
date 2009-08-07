/******************************************************************************
 *
 *  File : filterlistwidget.h
 *  Created on Fri 07 Aug 2009 03:23:10 by Szymon Tomasz Stefanek
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

#ifndef _FILTERLISTWIDGET_H_
#define _FILTERLISTWIDGET_H_

#include <QtGui/QListWidget>

class Filter;
class FilterListWidget;

class FilterListWidgetItem : public QListWidgetItem
{
private:
  Filter * mFilter; // owned, never null
public:
  FilterListWidgetItem( FilterListWidget * parent, Filter * filter );
  virtual ~FilterListWidgetItem();
public:
  Filter * filter()
  {
    return mFilter;
  }
};


class FilterListWidget : public QListWidget
{
  Q_OBJECT
public:
  FilterListWidget( QWidget * parent );
  virtual ~FilterListWidget();
};


#endif //!_FILTERLISTWIDGET_H_
