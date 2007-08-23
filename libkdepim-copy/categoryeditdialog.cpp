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

#include <QLayout>
#include <QStringList>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QBoxLayout>
#include <QList>
#include <QHeaderView>

#include <KLocale>

#include "autochecktreewidget.h"
#include "kpimprefs.h"
#include "categoryhierarchyreader.h"

#include "ui_categoryeditdialog_base.h"
#include "categoryeditdialog.h"

using namespace KPIM;

CategoryEditDialog::CategoryEditDialog( KPimPrefs *prefs, QWidget* parent )
  : KDialog( parent ), mPrefs( prefs )
{
  setCaption( i18n( "Edit Categories" ) );
  setButtons( Ok|Apply|Cancel|Help );
  mWidgets = new Ui::CategoryEditDialog_base();
  QWidget *widget = new QWidget( this );
  widget->setObjectName( "CategoryEdit" );
  mWidgets->setupUi( widget );

  QBoxLayout *layout = new QVBoxLayout( mWidgets->mCategoriesFrame );
  layout->setMargin( 0 );
  layout->setSpacing( KDialog::spacingHint() );
  mCategories = new AutoCheckTreeWidget( mWidgets->mCategoriesFrame );
  mCategories->setObjectName( "mCategories" );
  mCategories->setAutoCheck( false );
  mCategories->setHeaderLabel( i18n( "Category" ) );
  mCategories->setDragDropMode( QAbstractItemView::InternalMove );
  mCategories->setSelectionMode( QAbstractItemView::ExtendedSelection );
  mCategories->setAllColumnsShowFocus( true );
  mCategories->header()->hide();
  layout->addWidget( mCategories );

  // fix the tab order
  widget->setTabOrder( mCategories, mWidgets->mEdit );
  widget->setTabOrder( mWidgets->mEdit, mWidgets->mButtonAdd );
  widget->setTabOrder( mWidgets->mButtonAdd, mWidgets->mButtonAddSubcategory );
  widget->setTabOrder( mWidgets->mButtonAddSubcategory,
                        mWidgets->mButtonRemove );

  setMainWidget( widget );

  fillList();

  connect( mCategories, 
           SIGNAL( currentItemChanged( QTreeWidgetItem *, QTreeWidgetItem * )),
           SLOT( editItem( QTreeWidgetItem * )) );
  connect( mCategories, SIGNAL( itemSelectionChanged() ),
           SLOT( slotSelectionChanged() ) );
  connect( mCategories, SIGNAL( itemCollapsed( QTreeWidgetItem * ) ),
           SLOT( expandIfToplevel( QTreeWidgetItem * ) ) );
  connect( mWidgets->mEdit, SIGNAL( textChanged( const QString & )),
           this, SLOT( slotTextChanged( const QString & )));
  connect( mWidgets->mButtonAdd, SIGNAL( clicked() ),
           this, SLOT( add() ) );
  connect( mWidgets->mButtonAddSubcategory, SIGNAL( clicked() ),
           this, SLOT( addSubcategory() ) );
  connect( mWidgets->mButtonRemove, SIGNAL( clicked() ),
           this, SLOT( remove() ) );
  connect( this, SIGNAL( okClicked() ), this, SLOT( slotOk() ) );
  connect( this, SIGNAL( cancelClicked() ), this, SLOT( slotCancel() ) );
  connect( this, SIGNAL( applyClicked() ), this, SLOT( slotApply() ) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
CategoryEditDialog::~CategoryEditDialog()
{
  delete mWidgets;
}

void CategoryEditDialog::fillList()
{
  CategoryHierarchyReaderQTreeWidget( mCategories ).
      read( mPrefs->mCustomCategories );

  mWidgets->mButtonRemove->setEnabled( mCategories->topLevelItemCount() > 0 );
  mWidgets->mButtonAddSubcategory->setEnabled( mCategories->topLevelItemCount()
                                               > 0 );
}

void CategoryEditDialog::slotTextChanged(const QString &text)
{
  QTreeWidgetItem *item = mCategories->currentItem();
  if ( item ) {
    item->setText( 0, text );
  }
}

void CategoryEditDialog::slotSelectionChanged()
{
  QTreeWidgetItemIterator it( mCategories, QTreeWidgetItemIterator::Selected );
  mWidgets->mButtonRemove->setEnabled( *it );
}

void CategoryEditDialog::add()
{
  if ( !mWidgets->mEdit->text().isEmpty() ) {
    QTreeWidgetItem *newItem = 
      new QTreeWidgetItem( mCategories, QStringList( "" ) );
    // FIXME: Use a better string once string changes are allowed again
//                                                i18n("New category") );
    newItem->setExpanded( true );
    mCategories->setCurrentItem( newItem );
    mCategories->clearSelection();
    newItem->setSelected( true );
    mCategories->scrollToItem( newItem );
    mWidgets->mButtonRemove->setEnabled( mCategories->topLevelItemCount() > 0 );
    mWidgets->mButtonAddSubcategory->setEnabled( mCategories->topLevelItemCount() > 0 );
    mWidgets->mEdit->setFocus();
  }
}

void CategoryEditDialog::addSubcategory()
{
  if ( !mWidgets->mEdit->text().isEmpty() ) {
    QTreeWidgetItem *newItem = 
      new QTreeWidgetItem( mCategories->currentItem(), QStringList( "" ) );
    // FIXME: Use a better string once string changes are allowed again
//                                                i18n("New category") );
    newItem->setExpanded( true );
    mCategories->setCurrentItem( newItem );
    mCategories->clearSelection();
    newItem->setSelected( true );
    mCategories->scrollToItem( newItem );
    mWidgets->mEdit->setFocus();
  }
}

void CategoryEditDialog::remove()
{
  QList<QTreeWidgetItem*> to_remove;
  QTreeWidgetItemIterator it( mCategories, QTreeWidgetItemIterator::Selected );
  while ( *it ) {
    to_remove.append( *it++ );
  }

  QListIterator<QTreeWidgetItem*> items( to_remove );
  while ( items.hasNext() ) {
    delete items.next();
  }

  mWidgets->mButtonRemove->setEnabled( mCategories->topLevelItemCount() > 0 );
  mWidgets->mButtonAddSubcategory->setEnabled( mCategories->topLevelItemCount() > 0 );
  if ( mCategories->currentItem() )
    mCategories->currentItem()->setSelected( true );
}

void CategoryEditDialog::slotOk()
{
  slotApply();
  accept();
}

void CategoryEditDialog::slotApply()
{
  mPrefs->mCustomCategories.clear();

  QStringList path;
  QTreeWidgetItemIterator it( mCategories );
  while ( *it ) {
    path = mCategories->pathByItem( *it++ );
    path.replaceInStrings( KPimPrefs::categorySeparator, QString("\\") +
                           KPimPrefs::categorySeparator );
    mPrefs->mCustomCategories.append( path.join( KPimPrefs::categorySeparator ) );
  }

  mPrefs->writeConfig();

  emit categoryConfigChanged();
}

void CategoryEditDialog::slotCancel()
{
  reload();
}

void CategoryEditDialog::editItem( QTreeWidgetItem *item )
{
  if ( item )
    mWidgets->mEdit->setText( item->text( 0 ) );
}

void CategoryEditDialog::reload()
{
  fillList();
}

void CategoryEditDialog::show()
{
  QTreeWidgetItem *first = 0;
  if ( mCategories->topLevelItemCount() ) {
    first = mCategories->topLevelItem( 0 );
    mCategories->setCurrentItem( first );
  }
  mCategories->clearSelection();
  if ( first ) {
    first->setSelected( true );
    editItem( first );
  }
  KDialog::show();
}

void CategoryEditDialog::expandIfToplevel( QTreeWidgetItem *item )
{
  if ( !item->parent() )
    item->setExpanded( true );
}

#include "categoryeditdialog.moc"
