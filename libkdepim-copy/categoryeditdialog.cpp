/*
    This file is part of libkdepim.
    Copyright (c) 2000, 2001, 2002 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include <qstringlist.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qheader.h>
#include <qpushbutton.h>

#include "kpimprefs.h"

#include "categoryeditdialog.h"

using namespace KPIM;

CategoryEditDialog::CategoryEditDialog( KPimPrefs *prefs, QWidget* parent,
                                        const char* name, bool modal,
                                        WFlags fl )
  : CategoryEditDialog_base( parent, name, modal, fl ),
    mPrefs( prefs )  
{
  mCategories->header()->hide();

  QStringList::Iterator it;
  bool categoriesExist=false;
  for (it = mPrefs->mCustomCategories.begin();
       it != mPrefs->mCustomCategories.end(); ++it ) {
    new QListViewItem(mCategories,*it);
    categoriesExist=true;
  }

  connect(mCategories,SIGNAL(selectionChanged(QListViewItem *)),
          SLOT(editItem(QListViewItem *)));
  connect(mEdit,SIGNAL(textChanged ( const QString & )),this,SLOT(slotTextChanged(const QString &)));
  mButtonRemove->setEnabled(categoriesExist);
  mButtonModify->setEnabled(categoriesExist);
  mButtonAdd->setEnabled(!mEdit->text().isEmpty());
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
    mButtonAdd->setEnabled(!text.isEmpty());
}

void CategoryEditDialog::add()
{
  if (!mEdit->text().isEmpty()) {
    new QListViewItem(mCategories,mEdit->text());
    mEdit->setText("");
    mButtonRemove->setEnabled(mCategories->childCount()>0);
    mButtonModify->setEnabled(mCategories->childCount()>0);
  }
}

void CategoryEditDialog::remove()
{
  if (mCategories->currentItem()) {
    delete mCategories->currentItem();
    mButtonRemove->setEnabled(mCategories->childCount()>0);
    mButtonModify->setEnabled(mCategories->childCount()>0);
  }
}

void CategoryEditDialog::modify()
{
  if (!mEdit->text().isEmpty()) {
    if (mCategories->currentItem()) {
      mCategories->currentItem()->setText(0,mEdit->text());
    }
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

  QListViewItem *item = mCategories->firstChild();
  while(item) {
    mPrefs->mCustomCategories.append(item->text(0));
    item = item->nextSibling();
  }
  mPrefs->writeConfig();

  emit categoryConfigChanged();
}

void CategoryEditDialog::editItem(QListViewItem *item)
{
  mEdit->setText(item->text(0));
  mButtonRemove->setEnabled(true);
  mButtonModify->setEnabled(true);
}

#include "categoryeditdialog.moc"
