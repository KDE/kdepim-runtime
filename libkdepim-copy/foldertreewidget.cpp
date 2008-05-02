/******************************************************************************
 *
 * This file is part of libkdepim.
 *
 * Copyright (C) 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 *****************************************************************************/

#include "foldertreewidget.h"

#include <QHeaderView>

namespace KPIM {


FolderTreeWidget::FolderTreeWidget( QWidget *parent , const char *name )
: KPIM::TreeWidget( parent , name )
{
  setAlternatingRowColors( true );
  setAcceptDrops( true );
  setAllColumnsShowFocus( true );
  setRootIsDecorated( true );
  setSortingEnabled( true );
  header()->setSortIndicatorShown( true );
  header()->setClickable( true );
  //setSelectionMode( Extended );
}




FolderTreeWidgetItem::FolderTreeWidgetItem(
        FolderTreeWidget *parent,
        const QString &label,
        Protocol protocol,
        FolderType folderType
    ) : QTreeWidgetItem( parent ), mProtocol(protocol), mFolderType(folderType)
{
}

FolderTreeWidgetItem::FolderTreeWidgetItem(
        FolderTreeWidgetItem *parent,
        const QString &label,
        Protocol protocol,
        FolderType folderType,
        int unread,
        int total
    ) : QTreeWidgetItem( parent ), mProtocol(protocol), mFolderType(folderType)
{
}

bool FolderTreeWidgetItem::operator < ( const QTreeWidgetItem &other ) const
{
  int sortCol = treeWidget()->sortColumn();
  if ( sortCol < 0 )
     return true; // just "yes" :D

  // WIP

  return text(sortCol) < other.text(sortCol);
}

}
