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

#include <qlistview.h>
#include <qpushbutton.h>
#include <qheader.h>

#include "categoryselectdialog_base.h"
#include <klocale.h>
#include "categoryselectdialog.h"

#include "kpimprefs.h"

using namespace KPIM;

CategorySelectDialog::CategorySelectDialog( KPimPrefs *prefs, QWidget* parent,
                                            const char* name, bool modal )
  : KDialogBase::KDialogBase( parent, name, modal,
    i18n("Select Categories"), Ok|Apply|Cancel|Help, Ok, true ),
    mPrefs( prefs )
{
  mWidget = new CategorySelectDialog_base( this, "CategorySelection" );
  mWidget->mCategories->header()->hide();
  setMainWidget( mWidget );

  setCategories();
 
  connect( mWidget->mButtonEdit, SIGNAL(clicked()),
           SIGNAL(editCategories()) );
  connect( mWidget->mButtonClear, SIGNAL(clicked()),
           SLOT(clear()) );
}

void CategorySelectDialog::setCategories( const QStringList &categoryList )
{
  mWidget->mCategories->clear();
  mCategoryList.clear();

  QStringList::ConstIterator it;

  for ( it = categoryList.begin(); it != categoryList.end(); ++it )
    if ( mPrefs->mCustomCategories.find( *it ) == mPrefs->mCustomCategories.end() )
      mPrefs->mCustomCategories.append( *it );

  for ( it = mPrefs->mCustomCategories.begin();
        it != mPrefs->mCustomCategories.end(); ++it ) {
    new QCheckListItem( mWidget->mCategories, *it, QCheckListItem::CheckBox );
  }
}

CategorySelectDialog::~CategorySelectDialog()
{
}

void CategorySelectDialog::setSelected(const QStringList &selList)
{
  clear();

  QStringList::ConstIterator it;
  for ( it = selList.begin(); it != selList.end(); ++it ) {
    QCheckListItem *item = (QCheckListItem *)mWidget->mCategories->firstChild();
    while (item) {
      if (item->text() == *it) {
        item->setOn(true);
        break;
      }
      item = (QCheckListItem *)item->nextSibling();
    }
  }
}

QStringList CategorySelectDialog::selectedCategories() const
{
  return mCategoryList;
}

void CategorySelectDialog::slotApply()
{
  QStringList categories;
  QCheckListItem *item = (QCheckListItem *)mWidget->mCategories->firstChild();
  while (item) {
    if (item->isOn()) {
      categories.append(item->text());
    }
    item = (QCheckListItem *)item->nextSibling();
  }
  
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
  QCheckListItem *item = (QCheckListItem *)mWidget->mCategories->firstChild();
  while (item) {
    item->setOn(false);
    item = (QCheckListItem *)item->nextSibling();
  }  
}

void CategorySelectDialog::updateCategoryConfig()
{
  QStringList selected;
  QCheckListItem *item = (QCheckListItem *)mWidget->mCategories->firstChild();
  while (item) {
    if (item->isOn()) {
      selected.append(item->text());
    }
    item = (QCheckListItem *)item->nextSibling();
  }

  setCategories();
  
  setSelected(selected);
}

#include "categoryselectdialog.moc"
