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

#include "autoselectingchecklistitem.h"

AutoselectingCheckListItem::AutoselectingCheckListItem( Q3ListViewItem * parent, 
                                     const QString & text, 
                                     Q3CheckListItem::Type tt )
  : Q3CheckListItem( parent, text, tt ), mAutoselectChildren( false )
{
}

AutoselectingCheckListItem::AutoselectingCheckListItem( Q3ListView * parent, 
                                     const QString & text, 
                                     Q3CheckListItem::Type tt )
  : Q3CheckListItem( parent, text, tt ), mAutoselectChildren( false )
{
}

AutoselectingCheckListItem::~AutoselectingCheckListItem()
{
}

void AutoselectingCheckListItem::setAutoselectChildren(bool autoselectChildren )
{
  mAutoselectChildren = autoselectChildren;
}

bool AutoselectingCheckListItem::autoselectChildren() const
{
  return mAutoselectChildren;
}

void AutoselectingCheckListItem::stateChange( bool state )
{
  if ( mAutoselectChildren ) {
    for ( AutoselectingCheckListItem *item = 
          ( AutoselectingCheckListItem *) firstChild(); item; 
          item = ( AutoselectingCheckListItem *) item->nextSibling() )
      item->setOn( state );
  }
}
