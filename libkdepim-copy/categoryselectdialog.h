/*
    This file is part of libkdepim.

    Copyright (c) 2000, 2001, 2002 Cornelius Schumacher <schumacher@kde.org>
    Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/
#ifndef KPIM_CATEGORYSELECTDIALOG_H
#define KPIM_CATEGORYSELECTDIALOG_H

#include <kdialogbase.h>

class KPimPrefs;
class CategorySelectDialog_base;

namespace KPIM {

class CategorySelectDialog : public KDialogBase
{ 
    Q_OBJECT
  public:
    CategorySelectDialog( KPimPrefs *prefs, QWidget *parent = 0,
                          const char *name = 0, bool modal = false );
    ~CategorySelectDialog();

    /**
      Adds this categories to the default categories.
     */
    void setCategories( const QStringList &categoryList = QStringList() );
    void setSelected( const QStringList &selList );

    QStringList selectedCategories() const;
    
  public slots:
    void slotOk();
    void slotApply();
    void clear();
    void updateCategoryConfig();
    
  signals:
    void categoriesSelected( const QString & );
    void categoriesSelected( const QStringList & );
    void editCategories();

  private:
    KPimPrefs *mPrefs;
    CategorySelectDialog_base *mWidget;
    QStringList mCategoryList;

    class CategorySelectDialogPrivate;
    CategorySelectDialogPrivate *d;
};

}

#endif
