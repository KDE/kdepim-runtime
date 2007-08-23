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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#ifndef KPIM_CATEGORYSELECTDIALOG_H
#define KPIM_CATEGORYSELECTDIALOG_H

#include <KLocale>

#include <kdialog.h>
#include <kdepim_export.h>
#include "ui_categoryselectdialog_base.h"

class KPimPrefs;

namespace KPIM {
  class AutoCheckTreeWidget;
}

namespace KPIM {

class CategorySelectWidgetBase : public QWidget, public Ui::CategorySelectDialog_base
{
public:
  CategorySelectWidgetBase( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );

    // FIXME this seems not to work
    mCategories->setHeaderLabel( i18n( "Categories" ) );
  }
};


class KDEPIM_EXPORT CategorySelectWidget : public QWidget
{
    Q_OBJECT
  public:
    CategorySelectWidget(QWidget *parent, KPimPrefs *prefs);
    ~CategorySelectWidget();
    
    void setCategories( const QStringList &categoryList = QStringList() );
    void setSelected( const QStringList &selList );
    QStringList selectedCategories() const;    
    void setAutoselectChildren( bool autoselectChildren );
    void setCategoryList(const QStringList &categories);

    QStringList selectedCategories(QString & categoriesStr);

    void hideButton();
    void hideHeader();

    KPIM::AutoCheckTreeWidget *listView() const;

  public Q_SLOTS:
    void clear();

  Q_SIGNALS:
    void editCategories();

  private:
    QStringList mCategoryList;
    CategorySelectWidgetBase *mWidgets;
    KPimPrefs *mPrefs;
};

class KDEPIM_EXPORT CategorySelectDialog : public KDialog
{ 
    Q_OBJECT
  public:
    CategorySelectDialog( KPimPrefs *prefs, QWidget *parent = 0,
                          const char *name = 0, bool modal = false );
    ~CategorySelectDialog();

    QStringList selectedCategories() const;
    void setCategoryList(const QStringList &categories);

    void setAutoselectChildren( bool autoselectChildren );
    void setSelected( const QStringList &selList );

  public Q_SLOTS:
    void slotOk();
    void slotApply();
    void updateCategoryConfig();
    
  Q_SIGNALS:
    void categoriesSelected( const QString & );
    void categoriesSelected( const QStringList & );
    void editCategories();

  private:
    CategorySelectWidget *mWidgets;

    class CategorySelectDialogPrivate;
    CategorySelectDialogPrivate *d;
};

}

#endif
