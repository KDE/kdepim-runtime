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
#ifndef KPIMIMPROVEDLISTVIEW_H
#define KPIMIMPROVEDLISTVIEW_H

#include <k3listview.h>

namespace KPIM {

/**
A listview that controls expandibility and the ability to accept child items separately.

@author Rafal Rzepecki
*/
class ImprovedListView : public K3ListView
{
    Q_OBJECT
    Q_PROPERTY( bool leavesAcceptChildren READ leavesAcceptChildren
        WRITE setLeavesAcceptChildren )
  public:
    ImprovedListView( QWidget *parent = 0, const char *name = 0 );

    /**
     * @return true if leaves in this listview can accept children, and false
     * otherwise.
     */
    bool leavesAcceptChildren() const;
    /**
     * Sets whether leaves in this listview should accept children (on drag and
     * drop) even if not expandable. The default is false.
     */
    void setLeavesAcceptChildren( bool b );

  protected:
    /**
     * @reimp from K3ListView.
     */
    virtual void findDrop ( const QPoint &pos, Q3ListViewItem *&parent,
                            Q3ListViewItem *&after );
  private:
    bool mLeavesAcceptChildren;
};

}

#endif
