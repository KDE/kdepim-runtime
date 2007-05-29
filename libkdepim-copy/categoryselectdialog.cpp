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
#include <QVBoxLayout>
#include <q3header.h>

#include <klocale.h>
#include "categoryselectdialog.h"
#include "categoryhierarchyreader.h"
#include "autoselectingchecklistitem.h"

#include "kpimprefs.h"

using namespace KPIM;


CategorySelectWidget::CategorySelectWidget(QWidget *parent, KPimPrefs *prefs)
  : QWidget(parent), mPrefs( prefs )
{
  QHBoxLayout *topL = new QHBoxLayout(this);
  mWidgets = new CategorySelectWidgetBase(this);
  topL->addWidget(mWidgets);
  connect( mWidgets->mButtonEdit, SIGNAL(clicked()),
           SIGNAL(editCategories()) );
  connect( mWidgets->mButtonClear, SIGNAL(clicked()),
           SLOT(clear()) );
}

CategorySelectWidget::~CategorySelectWidget()
{
}

Q3ListView *CategorySelectWidget::listView() const
{
   return mWidgets->mCategories;
}

void CategorySelectWidget::hideButton()
{
  mWidgets->mButtonEdit->hide();
  mWidgets->mButtonClear->hide();
}

void CategorySelectWidget::setCategories( const QStringList &categoryList )
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

void CategorySelectWidget::setSelected(const QStringList &selList)
{
  clear();
  QStringList::ConstIterator it;
  for ( it = selList.begin(); it != selList.end(); ++it ) {
    QStringList path = CategoryHierarchyReader::path( *it );
    Q3CheckListItem *item = (Q3CheckListItem *)mWidgets->mCategories->firstChild();
    while (item) {
      if (item->text() == path.first()) {
        if ( path.count() == 1 ) {
          item->setOn(true);
          break;
        } else {
          item = (Q3CheckListItem *)item->firstChild();
          path.pop_front();
        }
      } else
        item = (Q3CheckListItem *)item->nextSibling();
    }
  }
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
      item = (Q3CheckListItem *)item->firstChild();
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

void CategorySelectWidget::clear()
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

void CategorySelectWidget::setAutoselectChildren( bool autoselectChildren )
{
  for ( Q3ListViewItemIterator it( mWidgets->mCategories ); it.current(); ++it)
    ( ( AutoselectingCheckListItem *) it.current() )->
            setAutoselectChildren( autoselectChildren );
}

void CategorySelectWidget::hideHeader()
{
  mWidgets->mCategories->header()->hide();
}


QStringList CategorySelectWidget::selectedCategories(QString & categoriesStr)
{
  mCategoryList = getSelectedCategories(listView());
  categoriesStr = mCategoryList.join(", ");
  return mCategoryList;
}

QStringList CategorySelectWidget::selectedCategories() const
{
  return mCategoryList;
}

void CategorySelectWidget::setCategoryList(const QStringList &categories)
{
   mCategoryList = categories;
}

CategorySelectDialog::CategorySelectDialog( KPimPrefs *prefs, QWidget* parent,
                                            const char* name, bool modal )
  : KDialog( parent )
{
  setCaption( i18n( "Select Categories" ) );
  setModal( modal );
  setButtons( Ok|Apply|Cancel|Help );
  setObjectName( name );
  QWidget *page = new QWidget;
  setMainWidget( page );
  QVBoxLayout *lay = new QVBoxLayout( page );

  mWidgets = new CategorySelectWidget(this,prefs);
  mWidgets->setObjectName( "CategorySelection" );
  mWidgets->hideHeader();
  lay->addWidget(mWidgets);

  mWidgets->setCategories();

  connect( mWidgets, SIGNAL(editCategories()),
           SIGNAL(editCategories()) );


  connect( this, SIGNAL( okClicked() ), this, SLOT( slotOk() ) );
  connect( this, SIGNAL( applyClicked() ), this, SLOT( slotApply() ) );
}

CategorySelectDialog::~CategorySelectDialog()
{
  delete mWidgets;
}

QStringList CategorySelectDialog::selectedCategories() const
{
  return mWidgets->selectedCategories();
}

void CategorySelectDialog::slotApply()
{
  QString categoriesStr;
  QStringList categories = mWidgets->selectedCategories(categoriesStr);
  emit categoriesSelected(categories);
  emit categoriesSelected(categoriesStr);
}

void CategorySelectDialog::slotOk()
{
  slotApply();
  accept();
}

void CategorySelectDialog::updateCategoryConfig()
{
  QString tmp;
  QStringList selected = mWidgets->selectedCategories(tmp);
  mWidgets->setCategories();
  mWidgets->setSelected(selected);
}

void CategorySelectDialog::setAutoselectChildren( bool autoselectChildren )
{
  mWidgets->setAutoselectChildren(autoselectChildren);
}

void CategorySelectDialog::setCategoryList(const QStringList &categories)
{
  mWidgets->setCategoryList(categories);
}

void CategorySelectDialog::setSelected( const QStringList &selList )
{
  mWidgets->setSelected(selList);
}


#include "categoryselectdialog.moc"
