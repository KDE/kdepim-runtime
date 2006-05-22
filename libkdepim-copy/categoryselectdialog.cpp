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

#include <q3listview.h>
#include <QPushButton>
#include <q3header.h>

#include "ui_categoryselectdialog_base.h"
#include <klocale.h>
#include "categoryselectdialog.h"
#include "categoryhierarchyreader.h"
#include "autoselectingchecklistitem.h"

#include "kpimprefs.h"

using namespace KPIM;

CategorySelectDialog::CategorySelectDialog( KPimPrefs *prefs, QWidget* parent,
                                            const char* name, bool modal )
  : KDialog( parent, i18n("Select Categories"), Ok|Apply|Cancel|Help  ),
    mPrefs( prefs )
{
  setModal( modal );
  setObjectName( name );
  mWidgets = new Ui::CategorySelectDialog_base();
  QWidget *widget = new QWidget( this );
  mWidgets->setupUi( widget );
  widget->setObjectName( "CategorySelection" );
  mWidgets->mCategories->header()->hide();
  setMainWidget( widget );

  setCategories();
 
  connect( mWidgets->mButtonEdit, SIGNAL(clicked()),
           SIGNAL(editCategories()) );
  connect( mWidgets->mButtonClear, SIGNAL(clicked()),
           SLOT(clear()) );
  connect( this, SIGNAL( okClicked() ), this, SLOT( slotOk() ) );
  connect( this, SIGNAL( applyClicked() ), this, SLOT( slotApply() ) );
}

void CategorySelectDialog::setCategories( const QStringList &categoryList )
{
  mWidgets->mCategories->clear();
  mCategoryList.clear();

  QStringList::ConstIterator it;

  for ( it = categoryList.begin(); it != categoryList.end(); ++it )
    if ( !mPrefs->mCustomCategories.contains( *it )  )
      mPrefs->mCustomCategories.append( *it );

  CategoryHierarchyReaderQListView( mWidgets->mCategories, false, true ).
      read( mPrefs->mCustomCategories );
}

CategorySelectDialog::~CategorySelectDialog()
{
  delete mWidgets;
}

void CategorySelectDialog::setSelected(const QStringList &selList)
{
  clear();

  QStringList::ConstIterator it;
  for ( it = selList.begin(); it != selList.end(); ++it ) {
    QStringList path = CategoryHierarchyReader::path( *it );
    Q3CheckListItem *item = (Q3CheckListItem *)mWidgets->mCategories->firstChild();
    while (item) {
     item = (Q3CheckListItem *)item->nextSibling();
      if (item->text() == path.first()) {
        if ( path.count() == 1 ) {
          item->setOn(true);
          break;
        } else {
          item = (Q3CheckListItem *)item->nextSibling();
          path.pop_front();
        }
      } else
        item = (Q3CheckListItem *)item->nextSibling();
    }
  }
}

QStringList CategorySelectDialog::selectedCategories() const
{
  return mCategoryList;
}

static QStringList getSelectedCategories( const Q3ListView *categoriesView )
{
  QStringList categories;
  Q3CheckListItem *item = (Q3CheckListItem *)categoriesView->firstChild();
  QStringList path;
  while (item) {
    path.append( item->text() );
    if (item->isOn()) {
      QStringList _path = path;
      _path.replaceInStrings( KPimPrefs::categorySeparator, QString( "\\" ) + 
                              KPimPrefs::categorySeparator );
      categories.append( _path.join( KPimPrefs::categorySeparator ) );
    }
    if ( item->firstChild() ) {
      item = (Q3CheckListItem *)item->nextSibling();
    } else {
      Q3CheckListItem *next_item = 0;
      while ( !next_item && item ) {
        path.pop_back();
        next_item = (Q3CheckListItem *)item->nextSibling();
        item = (Q3CheckListItem *)item->parent();
      }
      item = next_item;
    }
  }
  return categories;
}

void CategorySelectDialog::slotApply()
{
  QStringList categories = getSelectedCategories( mWidgets->mCategories );
  
  QString categoriesStr = categories.join(", ");

  mCategoryList = categories;

  emit categoriesSelected(categories);
  emit categoriesSelected(categoriesStr);
}

void CategorySelectDialog::slotOk()
{
  slotApply();
  accept();
}

void CategorySelectDialog::clear()
{
  Q3CheckListItem *item = (Q3CheckListItem *)mWidgets->mCategories->firstChild();
  while (item) {
    item->setOn( false );
    if ( item->firstChild() ) {
      item = (Q3CheckListItem *)item->firstChild();
    } else {
      Q3CheckListItem *next_item = 0;
      while ( !next_item && item ) {
        next_item = (Q3CheckListItem *)item->nextSibling();
        item = (Q3CheckListItem *)item->parent();
      }
      item = next_item;
    }
  }
}

void CategorySelectDialog::updateCategoryConfig()
{
  QStringList selected = getSelectedCategories( mWidgets->mCategories );

  setCategories();
  
  setSelected(selected);
}

void CategorySelectDialog::setAutoselectChildren( bool autoselectChildren )
{
  for ( Q3ListViewItemIterator it( mWidgets->mCategories ); it.current(); ++it)
    ( ( AutoselectingCheckListItem *) it.current() )->
            setAutoselectChildren( autoselectChildren );
}

#include "categoryselectdialog.moc"
