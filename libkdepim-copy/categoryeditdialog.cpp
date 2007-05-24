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
#include <Q3ListView>
#include <Q3Header>
#include <QPushButton>
#include <QVBoxLayout>
#include <QBoxLayout>
#include <Q3PtrList>
#include <klocale.h>

#include "improvedlistview.h"
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
  mCategories = new ImprovedListView( mWidgets->mCategoriesFrame );
  mCategories->setObjectName( "mCategories" );
  mCategories->addColumn( i18n( "Category" ) );
  mCategories->setAcceptDrops( true );
  mCategories->setProperty( "selectionMode", "Extended" );
  mCategories->setAllColumnsShowFocus( true );
  mCategories->setResizeMode( K3ListView::AllColumns );
  mCategories->setDragEnabled( true );
  mCategories->header()->hide();
  mCategories->setLeavesAcceptChildren( true );
  layout->addWidget( mCategories );

  // fix the tab order
  widget->setTabOrder( mCategories, mWidgets->mEdit );
  widget->setTabOrder( mWidgets->mEdit, mWidgets->mButtonAdd );
  widget->setTabOrder( mWidgets->mButtonAdd, mWidgets->mButtonAddSubcategory );
  widget->setTabOrder( mWidgets->mButtonAddSubcategory,
                        mWidgets->mButtonRemove );

  setMainWidget( widget );

  fillList();

  connect( mCategories, SIGNAL( currentChanged( Q3ListViewItem * )),
           SLOT( editItem( Q3ListViewItem * )) );
  connect( mCategories, SIGNAL( selectionChanged() ),
           SLOT( slotSelectionChanged() ) );
  connect( mCategories, SIGNAL( collapsed( Q3ListViewItem * ) ),
           SLOT( expandIfToplevel( Q3ListViewItem * ) ) );
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
  CategoryHierarchyReaderQListView( mCategories, false ).
      read( mPrefs->mCustomCategories );

  mWidgets->mButtonRemove->setEnabled( mCategories->childCount() > 0 );
  mWidgets->mButtonAddSubcategory->setEnabled( mCategories->childCount()
                                               > 0 );
}

void CategoryEditDialog::slotTextChanged(const QString &text)
{
  Q3ListViewItem *item = mCategories->currentItem();
  if ( item ) {
    item->setText( 0, text );
  }
}

void CategoryEditDialog::slotSelectionChanged()
{
  Q3ListViewItem *item = mCategories->firstChild();
  while (item) {
    if ( item->isSelected() ) {
      mWidgets->mButtonRemove->setEnabled( true );
      return;
    }
    if ( item->firstChild() )
      item = item->firstChild();
    else while ( item ) {
      if ( item->nextSibling() ) {
        item = item->nextSibling();
        break;
      }
      item = item->parent();
    }
  }
  mWidgets->mButtonRemove->setEnabled( false );
}

void CategoryEditDialog::add()
{
  if ( !mWidgets->mEdit->text().isEmpty() ) {
    Q3ListViewItem *newItem = new Q3ListViewItem( mCategories, "" );
    // FIXME: Use a better string once string changes are allowed again
//                                                i18n("New category") );
    newItem->setOpen( true );
    mCategories->setCurrentItem( newItem );
    mCategories->clearSelection();
    mCategories->setSelected( newItem, true );
    mCategories->ensureItemVisible( newItem );
    mWidgets->mButtonRemove->setEnabled( mCategories->childCount()>0 );
    mWidgets->mButtonAddSubcategory->setEnabled( mCategories->childCount()>0 );
    mWidgets->mEdit->setFocus();
  }
}

void CategoryEditDialog::addSubcategory()
{
  if ( !mWidgets->mEdit->text().isEmpty() ) {
    Q3ListViewItem *newItem = new Q3ListViewItem( mCategories->
                                                currentItem(), "" );
    // FIXME: Use a better string once string changes are allowed again
//                                                i18n("New category") );
    newItem->setOpen( true );
    mCategories->setCurrentItem( newItem );
    mCategories->clearSelection();
    mCategories->setSelected( newItem, true );
    mCategories->ensureItemVisible( newItem );
    mWidgets->mEdit->setFocus();
  }
}

void CategoryEditDialog::remove()
{
  Q3ListViewItem *item = mCategories->firstChild();
  Q3PtrList<Q3ListViewItem> to_remove;
  bool subs = false;
  while (item) {
    if ( item->isSelected() ) {
      to_remove.append( item );
      if ( item->childCount() > 0 )
        subs = true;
    }
    if ( item->firstChild() )
      item = item->firstChild();
    else while ( item ) {
      if ( item->nextSibling() ) {
        item = item->nextSibling();
        break;
      }
      item = item->parent();
    }
  }
/*  if ( subs && KMessageBox::warningYesNo( this, i18n("The subcategories will "
                                                     "also be removed. Are you "
                                                     "sure?") )
          == KMessageBox::No )
    return;*/ // no need for the message box since the dialog is cancellable
  // we run backwards to delete children before parents
  for ( Q3ListViewItem *it = to_remove.last(); it; it = to_remove.prev())
    delete it;
  mWidgets->mButtonRemove->setEnabled( mCategories->childCount()>0 );
  mWidgets->mButtonAddSubcategory->setEnabled( mCategories->childCount()>0 );
  if ( mCategories->childCount()>0 )
    mCategories->setSelected( mCategories->currentItem(), true );
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
  Q3ListViewItem *item = mCategories->firstChild();
  while ( item ) {
    path.append( item->text(0) );
    QStringList _path = path;
    _path.replaceInStrings( KPimPrefs::categorySeparator, QString("\\") +
                            KPimPrefs::categorySeparator );
    mPrefs->mCustomCategories.append( _path.join(KPimPrefs::categorySeparator) );
    if ( item->firstChild() ) {
      item = item->firstChild();
    } else {
      Q3ListViewItem *next_item = 0;
      while ( !next_item && item ) {
        path.pop_back();
        next_item = item->nextSibling();
        item = item->parent();
      }
      item = next_item;
    }
  }
  mPrefs->writeConfig();

  emit categoryConfigChanged();
}

void CategoryEditDialog::slotCancel()
{
  reload();
}

void CategoryEditDialog::editItem( Q3ListViewItem *item )
{
  if ( item )
    mWidgets->mEdit->setText( item->text(0) );
}

void CategoryEditDialog::reload()
{
  fillList();
}

void CategoryEditDialog::show()
{
  Q3ListViewItem *first = mCategories->firstChild();
  mCategories->setCurrentItem( first );
  mCategories->clearSelection();
  if ( first ) {
    mCategories->setSelected( first, true );
    editItem( first );
  }
  KDialog::show();
}

void CategoryEditDialog::expandIfToplevel( Q3ListViewItem *item )
{
  if ( !item->parent() )
    item->setOpen( true );
}

#include "categoryeditdialog.moc"
