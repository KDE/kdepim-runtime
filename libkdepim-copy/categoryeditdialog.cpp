/*
    This file is part of libkdepim.

    Copyright (c) 2000, 2001, 2002 Cornelius Schumacher <schumacher@kde.org>

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

#include <qstringlist.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qheader.h>
#include <qpushbutton.h>
#include <klocale.h>

#include "kpimprefs.h"

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
  mWidget->mCategories->header()->hide();
  setMainWidget( mWidget );

  QStringList::Iterator it;
  bool categoriesExist=false;
  for ( it = mPrefs->mCustomCategories.begin();
        it != mPrefs->mCustomCategories.end(); ++it ) {
    new QListViewItem( mWidget->mCategories, *it );
    categoriesExist = true;
  }
  mWidget->mCategories->setSelected( mWidget->mCategories->firstChild(), true );

  connect( mWidget->mCategories, SIGNAL( selectionChanged( QListViewItem * )),
           SLOT( editItem( QListViewItem * )) );
  connect( mWidget->mEdit, SIGNAL( textChanged( const QString & )),
           this, SLOT( slotTextChanged( const QString & )));
  connect( mWidget->mButtonAdd, SIGNAL( clicked() ),
           this, SLOT( add() ) );
  connect( mWidget->mButtonRemove, SIGNAL( clicked() ),
           this, SLOT( remove() ) );
  
  mWidget->mButtonRemove->setEnabled( categoriesExist );
}

/*
 *  Destroys the object and frees any allocated resources
 */
CategoryEditDialog::~CategoryEditDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

void CategoryEditDialog::slotTextChanged(const QString &text)
{
  QListViewItem *item = mWidget->mCategories->currentItem();
  if ( item ) {
    item->setText( 0, text );
  }
}

void CategoryEditDialog::add()
{
  if ( !mWidget->mEdit->text().isEmpty() ) {
    QListViewItem *newItem = new QListViewItem( mWidget->mCategories,
                                                i18n("New category") );
    mWidget->mCategories->setSelected( newItem, true );
    mWidget->mCategories->ensureItemVisible( newItem );
    mWidget->mButtonRemove->setEnabled( mWidget->mCategories->childCount()>0 );
  }
}

void CategoryEditDialog::remove()
{
  if (mWidget->mCategories->currentItem()) {
    delete mWidget->mCategories->currentItem();
    mWidget->mButtonRemove->setEnabled( mWidget->mCategories->childCount()>0 );
  }
}

void CategoryEditDialog::slotOk()
{
  slotApply();
  accept();
}

void CategoryEditDialog::slotApply()
{
  mPrefs->mCustomCategories.clear();

  QListViewItem *item = mWidget->mCategories->firstChild();
  while ( item ) {
    mPrefs->mCustomCategories.append( item->text(0) );
    item = item->nextSibling();
  }
  mPrefs->writeConfig();

  emit categoryConfigChanged();
}

void CategoryEditDialog::editItem( QListViewItem *item )
{
  mWidget->mEdit->setText( item->text(0) );
  mWidget->mButtonRemove->setEnabled( true );
}

void CategoryEditDialog::reload() 
{
  QStringList::Iterator it;
  mWidget->mCategories->clear();
  for ( it = mPrefs->mCustomCategories.begin();
        it != mPrefs->mCustomCategories.end(); ++it ) {
    new QListViewItem( mWidget->mCategories, *it );
  }
}

#include "categoryeditdialog.moc"
