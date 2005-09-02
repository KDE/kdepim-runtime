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

#ifndef KPIM_AUTOSELECTINGCHECKLISTITEM_H
#define KPIM_AUTOSELECTINGCHECKLISTITEM_H

#include <q3listview.h>

class AutoselectingCheckListItem : public Q3CheckListItem
{
  public:
    AutoselectingCheckListItem( Q3ListViewItem * parent, const QString & text, Q3CheckListItem::Type tt = Q3CheckListItem::RadioButtonController );
    AutoselectingCheckListItem( Q3ListView * parent, const QString & text, Q3CheckListItem::Type tt = Q3CheckListItem::RadioButtonController );
    virtual ~AutoselectingCheckListItem();
    void setAutoselectChildren( bool autoselectChildren );
    bool autoselectChildren() const;
    
  protected:
    virtual void stateChange( bool state );
  private:
    bool mAutoselectChildren;
};

#endif
