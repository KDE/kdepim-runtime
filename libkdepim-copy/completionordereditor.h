/* -*- c++ -*-
 * completionordereditor.h
 *
 *  Copyright (c) 2004 David Faure <faure@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */

#ifndef COMPLETIONORDEREDITOR_H
#define COMPLETIONORDEREDITOR_H

#include <kdialogbase.h>
#include <kconfig.h>
//Added by qt3to4:
#include <Q3PtrList>

class Q3ListViewItem;
class KPushButton;
class KListView;
namespace KPIM {

class LdapSearch;
class CompletionOrderEditor;

// Base class for items in the list
class CompletionItem
{
public:
  virtual ~CompletionItem() {}
  virtual QString label() const = 0;
  virtual int completionWeight() const = 0;
  virtual void setCompletionWeight( int weight ) = 0;
  virtual void save( CompletionOrderEditor* ) = 0;
};


// I don't like QPtrList much, but it has compareItems, which QValueList doesn't
class CompletionItemList : public Q3PtrList<CompletionItem>
{
public:
  CompletionItemList() {}
  virtual int compareItems( Q3PtrCollection::Item s1, Q3PtrCollection::Item s2 );
};

class CompletionOrderEditor : public KDialogBase {
  Q_OBJECT

public:
  CompletionOrderEditor( KPIM::LdapSearch* ldapSearch, QWidget* parent, const char* name = 0 );
  ~CompletionOrderEditor();

  KConfig* configFile() { return &mConfig; }

private slots:
  void slotSelectionChanged( Q3ListViewItem* );
  void slotMoveUp();
  void slotMoveDown();
  virtual void slotOk();

private:
  KConfig mConfig;
  CompletionItemList mItems;
  KListView* mListView;
  KPushButton* mUpButton;
  KPushButton* mDownButton;

  bool mDirty;
};

} // namespace

#endif /* COMPLETIONORDEREDITOR_H */

