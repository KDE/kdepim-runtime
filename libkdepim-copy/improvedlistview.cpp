/*
    This file is part of libkdepim.

    Copyright (c) 2005 Rafal Rzepecki <divide@users.sourceforge.net>
    Portions copyright (c) by kdelibs developers.

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
#include "improvedlistview.h"

namespace KPIM {
ImprovedListView::ImprovedListView( QWidget *parent, const char *name )
  : KListView( parent, name ), mLeavesAcceptChildren( false ) {}

bool ImprovedListView::leavesAcceptChildren() const
{
  return mLeavesAcceptChildren;
}

void ImprovedListView::setLeavesAcceptChildren( bool b )
{
  mLeavesAcceptChildren = b;
}

/* Just copied from kdelibs/kdeui/klistview.cpp with one small change. */
void ImprovedListView::findDrop ( const QPoint &pos, Q3ListViewItem *&parent,
                                  Q3ListViewItem *&after )
{
  QPoint p (contentsToViewport(pos));

        // Get the position to put it in
  Q3ListViewItem *atpos = itemAt(p);

  Q3ListViewItem *above;
  if (!atpos) // put it at the end
    above = lastItem();
  else
  {
                // Get the closest item before us ('atpos' or the one above, if any)
    if (p.y() - itemRect(atpos).topLeft().y() < (atpos->height()/2))
      above = atpos->itemAbove();
    else
      above = atpos;
  }

  if (above)
  {
                // if above has children, I might need to drop it as the first item there

    if (above->firstChild() && above->isOpen())
    {
      parent = above;
      after = 0;
      return;
    }

      // Now, we know we want to go after "above". But as a child or as a sibling ?
      // We have to ask the "above" item if it accepts children.
    if (mLeavesAcceptChildren || above->isExpandable())
    {
          // The mouse is sufficiently on the right ? - doesn't matter if 'above' has visible children
      if (p.x() >= depthToPixels( above->depth() + 1 ) ||
          (above->isOpen() && above->childCount() > 0) )
      {
        parent = above;
        after = 0L;
        return;
      }
    }

      // Ok, there's one more level of complexity. We may want to become a new
      // sibling, but of an upper-level group, rather than the "above" item
    Q3ListViewItem * betterAbove = above->parent();
    Q3ListViewItem * last = above;
    while ( betterAbove )
    {
          // We are allowed to become a sibling of "betterAbove" only if we are
          // after its last child
      if ( !last->nextSibling() )
      {
        if (p.x() < depthToPixels ( betterAbove->depth() + 1 ))
          above = betterAbove; // store this one, but don't stop yet, there may be a better one
        else
          break; // not enough on the left, so stop
        last = betterAbove;
        betterAbove = betterAbove->parent(); // up one level
      } else
        break; // we're among the child of betterAbove, not after the last one
    }
  }
  // set as sibling
  after = above;
  parent = after ? after->parent() : 0L ;
}

}

#include "improvedlistview.moc"
