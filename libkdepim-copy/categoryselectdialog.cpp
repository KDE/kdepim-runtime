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

#include <QPushButton>
#include <QVBoxLayout>
#include <QHeaderView>

#include <KLocale>
#include "categoryselectdialog.h"
#include "categoryhierarchyreader.h"
#include "autoselectingchecklistitem.h"
#include "autochecktreewidget.h"

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

KPIM::AutoCheckTreeWidget *CategorySelectWidget::listView() const
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

  CategoryHierarchyReaderAutoCheckTreeWidget( mWidgets->mCategories ).
      read( mPrefs->mCustomCategories );
}

void CategorySelectWidget::setSelected(const QStringList &selList)
{
  clear();
  QStringList::ConstIterator it;

  bool remAutoCheckChildren = mWidgets->mCategories->autoCheckChildren();
  mWidgets->mCategories->setAutoCheckChildren( false );
  for ( it = selList.begin(); it != selList.end(); ++it ) {
    QStringList path = CategoryHierarchyReader::path( *it );
    QTreeWidgetItem *item = mWidgets->mCategories->itemByPath( path );
    if ( item ) {
      item->setCheckState( 0, Qt::Checked );
    }
  }
  mWidgets->mCategories->setAutoCheckChildren( remAutoCheckChildren );
}

static QStringList getSelectedCategories( AutoCheckTreeWidget *categoriesView )
{
  QStringList categories;

  QTreeWidgetItemIterator it( categoriesView, QTreeWidgetItemIterator::Checked );
  while ( *it ) {
    QStringList path = categoriesView->pathByItem( *it++ );
    if ( path.count() ) {
      path.replaceInStrings( KPimPrefs::categorySeparator, QString( "\\" ) +
                             KPimPrefs::categorySeparator );
      categories.append( path.join( KPimPrefs::categorySeparator ) );
    }
  }

  return categories;
}

void CategorySelectWidget::clear()
{
  bool remAutoCheckChildren = mWidgets->mCategories->autoCheckChildren();
  mWidgets->mCategories->setAutoCheckChildren( false );

  QTreeWidgetItemIterator it( mWidgets->mCategories );
  while ( *it ) {
    ( *it++ )->setCheckState( 0, Qt::Unchecked );
  }

  mWidgets->mCategories->setAutoCheckChildren( remAutoCheckChildren );
}

void CategorySelectWidget::setAutoselectChildren( bool autoselectChildren )
{
  mWidgets->mCategories->setAutoCheckChildren( autoselectChildren );
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
