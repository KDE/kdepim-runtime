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

#include <qlayout.h>
#include <qstringlist.h>
#include <qlineedit.h>
#include <q3listview.h>
#include <q3header.h>
#include <qpushbutton.h>
#include <klocale.h>
// #include <kmessagebox.h>

#include "improvedlistview.h"
#include "kpimprefs.h"
#include "categoryhierarchyreader.h"

#include "categoryeditdialog_base.h"
#include "categoryeditdialog.h"

using namespace KPIM;

CategoryEditDialog::CategoryEditDialog( KPimPrefs *prefs, QWidget* parent,
                                        const char* name, bool modal )
  : KDialogBase::KDialogBase( parent, name, modal,
    i18n("Edit Categories"), Ok|Apply|Cancel|Help, Ok, true ),
    mPrefs( prefs )  
{
  mWidget = new CategoryEditDialog_base( this, "CategoryEdit" );

  QBoxLayout *layout = new QVBoxLayout( mWidget->mCategoriesFrame );
  mCategories = new ImprovedListView( mWidget->mCategoriesFrame,
                                      "mCategories" );
  mCategories->addColumn( i18n( "Category" ) );
  mCategories->setAcceptDrops( true );
  mCategories->setProperty( "selectionMode", "Extended" );
  mCategories->setAllColumnsShowFocus( true );
  mCategories->setResizeMode( KListView::AllColumns );
  mCategories->setDragEnabled( true );
  mCategories->header()->hide();
  mCategories->setLeavesAcceptChildren( true );
  layout->addWidget( mCategories );

  // fix the tab order
  mWidget->setTabOrder( mCategories, mWidget->mEdit );
  mWidget->setTabOrder( mWidget->mEdit, mWidget->mButtonAdd );
  mWidget->setTabOrder( mWidget->mButtonAdd, mWidget->mButtonAddSubcategory );
  mWidget->setTabOrder( mWidget->mButtonAddSubcategory,
                        mWidget->mButtonRemove );
  
  setMainWidget( mWidget );

  fillList();
  
  connect( mCategories, SIGNAL( currentChanged( Q3ListViewItem * )),
           SLOT( editItem( QListViewItem * )) );
  connect( mCategories, SIGNAL( selectionChanged() ),
           SLOT( slotSelectionChanged() ) );
  connect( mCategories, SIGNAL( collapsed( QListViewItem * ) ),
           SLOT( expandIfToplevel( QListViewItem * ) ) );
  connect( mWidget->mEdit, SIGNAL( textChanged( const QString & )),
           this, SLOT( slotTextChanged( const QString & )));
  connect( mWidget->mButtonAdd, SIGNAL( clicked() ),
           this, SLOT( add() ) );
  connect( mWidget->mButtonAddSubcategory, SIGNAL( clicked() ),
           this, SLOT( addSubcategory() ) );
  connect( mWidget->mButtonRemove, SIGNAL( clicked() ),
           this, SLOT( remove() ) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
CategoryEditDialog::~CategoryEditDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

void CategoryEditDialog::fillList()
{
  CategoryHierarchyReaderQListView( mCategories, false ).
      read( mPrefs->mCustomCategories );
  
  mWidget->mButtonRemove->setEnabled( mCategories->childCount() > 0 );
  mWidget->mButtonAddSubcategory->setEnabled( mCategories->childCount()
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
  QListViewItem *item = mCategories->firstChild();
  while (item) {
    if ( item->isSelected() ) {
      mWidget->mButtonRemove->setEnabled( true );
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
  mWidget->mButtonRemove->setEnabled( false );
}

void CategoryEditDialog::add()
{
  if ( !mWidget->mEdit->text().isEmpty() ) {
    Q3ListViewItem *newItem = new Q3ListViewItem( mCategories, "" );
    // FIXME: Use a better string once string changes are allowed again
//                                                i18n("New category") );
    newItem->setOpen( true );
    mCategories->setCurrentItem( newItem );
    mCategories->clearSelection();
    mCategories->setSelected( newItem, true );
    mCategories->ensureItemVisible( newItem );
    mWidget->mButtonRemove->setEnabled( mCategories->childCount()>0 );
    mWidget->mButtonAddSubcategory->setEnabled( mCategories->childCount()>0 );
    mWidget->mEdit->setFocus();
  }
}

void CategoryEditDialog::addSubcategory()
{
  if ( !mWidget->mEdit->text().isEmpty() ) {
    QListViewItem *newItem = new QListViewItem( mCategories->
                                                currentItem(), "" );
    // FIXME: Use a better string once string changes are allowed again
//                                                i18n("New category") );
    newItem->setOpen( true );
    mCategories->setCurrentItem( newItem );
    mCategories->clearSelection();
    mCategories->setSelected( newItem, true );
    mCategories->ensureItemVisible( newItem );
    mWidget->mEdit->setFocus();
  }
}

void CategoryEditDialog::remove()
{
  QListViewItem *item = mCategories->firstChild();
  QPtrList<QListViewItem> to_remove;
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
  for ( QListViewItem *it = to_remove.last(); it; it = to_remove.prev())
    delete it;
  mWidget->mButtonRemove->setEnabled( mCategories->childCount()>0 );
  mWidget->mButtonAddSubcategory->setEnabled( mCategories->childCount()>0 );
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
    _path.gres( KPimPrefs::categorySeparator, QString("\\") + 
                KPimPrefs::categorySeparator );
    mPrefs->mCustomCategories.append( _path.join(KPimPrefs::categorySeparator) );
    if ( item->firstChild() ) {
      item = item->firstChild();
    } else {
      QListViewItem *next_item = 0;
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
  KDialogBase::slotCancel();
}

void CategoryEditDialog::editItem( Q3ListViewItem *item )
{
  if ( item )
    mWidget->mEdit->setText( item->text(0) );
}

void CategoryEditDialog::reload() 
{
  fillList();
}

void CategoryEditDialog::show()
{
  QListViewItem *first = mCategories->firstChild();
  mCategories->setCurrentItem( first );
  mCategories->clearSelection();
  if ( first ) {
    mCategories->setSelected( first, true );
    editItem( first );
  }
  KDialog::show();
}

void CategoryEditDialog::expandIfToplevel( QListViewItem *item )
{
  if ( !item->parent() )
    item->setOpen( true );
}

#include "categoryeditdialog.moc"
