#ifndef __FOLDERTREEWIDGET_H__
#define __FOLDERTREEWIDGET_H__

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

/**
 * @file foldertreewidget.h A basic implementation of an UI for a tree of folders
 */

#include "kdepim_export.h"

#include "treewidget.h"

namespace KPIM
{

/**
 * @brief A tree widget useful for displaying a tree of folders containing messages
 *
 *
 * Work In Progress Warning:
 * 
 * This widget is meant to be a Native-Qt4 polished replacement for
 * KDEPIM::KFolderTreeWidget which is based on K3ListView.
 * Necessary features are being added as existing clients are ported.
 * This approach is taken in order to isolate features that are no
 * longer necessary and can be dropped from the new implementation. Take care :)
 *
 * @author: Szymon Tomasz Stefanek <pragma@kvirc.net>
 */
class KDEPIM_EXPORT FolderTreeWidget : public KPIM::TreeWidget
{
  Q_OBJECT
public:
  /**
   * Constructs a FolderTreeWidget instance.
   */
  explicit FolderTreeWidget( QWidget *parent , const char *name = 0 );

};


/**
 * @brief A folder tree node to be used with FolderTreeWidget
 *
 * @author: Szymon Tomasz Stefanek <pragma@kvirc.net>
 */
class KDEPIM_EXPORT FolderTreeWidgetItem : public QTreeWidgetItem
{
public:
  /**
   * Protocol information associated to the item.
   */
  enum Protocol {
      Imap,
      Local,
      News,
      CachedImap,
      Search,
      NONE
  };

  /**
   * Folder type information
   */
  enum FolderType {
      Inbox,
      Outbox,
      SentMail,
      Trash,
      Drafts,
      Templates,
      Root,
      Calendar,
      Tasks,
      Journals,
      Contacts,
      Notes,
      Other
  };

private:
  Protocol mProtocol;         ///< The protocol associated to the folder
  FolderType mFolderType;     ///< The type of the folder

public:
  /**
   * Constructs a root-item
   */
  explicit FolderTreeWidgetItem(
      FolderTreeWidget *parent,
      const QString &label = QString(),
      Protocol protocol = NONE,
      FolderType folderType = Root
    );

  /**
   * Constructs a child-item
   */
  explicit FolderTreeWidgetItem(
      FolderTreeWidgetItem *parent,
      const QString &label = QString(),
      Protocol protocol = NONE,
      FolderType folderType = Other,
      int unread = 0,
      int total = 0
   );

public:
  /**
   * Returns the protocol associated to the folder item
   */
  Protocol protocol() const
    { return mProtocol; };

  /**
   * Sets the protocol associated to the folder item
   */
  void setProtocol( Protocol protocol )
    { mProtocol = protocol; };

  /**
   * Returns the type of the folder
   */
  FolderType folderType() const
    { return mFolderType; };

  /**
   * Sets the type of the folder
   */
  void setFolderType( FolderType folderType )
    { mFolderType = folderType; };

  /**
   * Operator used for item sorting.
   */
  virtual bool operator < ( const QTreeWidgetItem &other ) const;

};

} // namespace KPIM

#endif //!__FOLDERTREEWIDGET_H__
