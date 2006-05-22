/*
    This file is part of libkdepim.

    Copyright (c) 2000, 2001, 2002 Cornelius Schumacher <schumacher@kde.org>
    Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
    Copyright (c) 2005 Rafal Rzepecki <divide@users.sourceforge.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#ifndef KPIM_CATEGORYEDITDIALOG_H
#define KPIM_CATEGORYEDITDIALOG_H

#include <kdialog.h>
#include <kdepimmacros.h>
#include <Q3ListViewItem>
class KPimPrefs;
namespace Ui {
class CategoryEditDialog_base;
}
namespace KPIM {

class ImprovedListView;

class KDE_EXPORT CategoryEditDialog : public KDialog
{
    Q_OBJECT
  public:
    CategoryEditDialog( KPimPrefs *prefs, QWidget* parent = 0 );
    ~CategoryEditDialog();

  public slots:
    void reload();
    virtual void show();

  protected slots:
    void slotOk();
    void slotApply();
    void slotCancel();
    void slotTextChanged(const QString &text);
    void slotSelectionChanged();
    void add();
    void addSubcategory();
    void remove();
    void editItem( Q3ListViewItem *item );
    void expandIfToplevel( Q3ListViewItem *item );

  signals:
    void categoryConfigChanged();
    
  protected:
    void fillList();

  private:
    ImprovedListView* mCategories;
    KPimPrefs *mPrefs;
    Ui::CategoryEditDialog_base *mWidgets;
};

}

#endif
