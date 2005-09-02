/*
    This file is part of libkdepim.

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

#include <qcombobox.h>
#include <qstringlist.h>
#include <q3listview.h>

#include "autoselectingchecklistitem.h"
#include "categoryhierarchyreader.h"
#include "kpimprefs.h"

using namespace KPIM;

inline QString &quote( QString &string )
{
  Q_ASSERT( KPimPrefs::categorySeparator != "@" );
  return string.replace( "@", "@0" ).replace( QString("\\") + 
                                              KPimPrefs::categorySeparator, 
                                              "@1" );
}

inline QStringList &unquote( QStringList &strings )
{
  return strings.gres( "@1", KPimPrefs::categorySeparator ).gres( "@0", "@" );
}

QStringList CategoryHierarchyReader::path( QString string )
{
  QStringList _path = QStringList::split( KPimPrefs::categorySeparator, 
                                         quote( string ) );
  return unquote( _path );
}

void CategoryHierarchyReader::read( QStringList categories )
{
  clear();
  QStringList::Iterator it;
  categories.sort();
  QStringList last_path;
  for ( it = categories.begin(); it != categories.end(); ++it ) {
    QStringList _path = path( *it );
    
    // we need to figure out where last item and the new one differ
    QStringList::Iterator jt, kt;
    int split_level = 0;
    QStringList new_path = _path; // save it for later
    for (jt = _path.begin(), kt = last_path.begin(); jt != _path.end() && kt != last_path.end(); ++jt, ++kt)
      if (*jt == *kt) {
        split_level++;
      } else
        break; // now we have first non_equal component in the iterators
    
    // make a path relative to the shared ancestor
    if ( jt != _path.begin() )
      _path.erase( _path.begin(), jt );
    last_path = new_path;

    if ( _path.isEmpty() ) {
      // something is wrong, we already have this node
      continue;
    }
    
    // find that ancestor
    while ( split_level < depth() ) {
      goUp();
    }
    Q_ASSERT( split_level == depth() );
    
    // make the node and any non-existent ancestors
    while ( !_path.isEmpty() ) {
      addChild( _path.first() );
      _path.pop_front();
    }
  }
}

void CategoryHierarchyReaderQListView::clear()
{
  mView->clear();
}

void CategoryHierarchyReaderQListView::goUp()
{
  Q_ASSERT( mItem );
  mItem = mItem->parent();
}

void CategoryHierarchyReaderQListView::addChild( const QString &label )
{
  if ( mCheckList ) {
    if ( mItem )
      mItem = (Q3ListViewItem *)new AutoselectingCheckListItem( mItem, label, 
                                                   Q3CheckListItem::CheckBox );
    else
      mItem = (Q3ListViewItem *)new AutoselectingCheckListItem( mView, label, 
                                                   Q3CheckListItem::CheckBox );
  } else {
    if ( mItem )
      mItem = new Q3ListViewItem( mItem, label );
    else
      mItem = new Q3ListViewItem( mView, label );
  }
  mItem->setExpandable( mExpandable );
  mItem->setOpen( true );
}

int CategoryHierarchyReaderQListView::depth() const
{
  if ( mItem )
    return mItem->depth() + 1;
  else
    return 0;
}

void CategoryHierarchyReaderQComboBox::clear()
{
  mBox->clear();
}

void CategoryHierarchyReaderQComboBox::goUp()
{
  mCurrentDepth--;
}

void CategoryHierarchyReaderQComboBox::addChild( const QString &label )
{
  QString spaces;
  spaces.fill( ' ', 2 * mCurrentDepth );
  mBox->insertItem( spaces + label );
  mCurrentDepth++;
}

int CategoryHierarchyReaderQComboBox::depth() const
{
  return mCurrentDepth;
}
