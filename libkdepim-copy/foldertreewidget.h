#ifndef KDEPIM_FOLDERTREEWIDGET_H
#define KDEPIM_FOLDERTREEWIDGET_H

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

#include <QHash>
#include <QColor>

namespace KPIM
{

class FolderTreeWidgetItem;

/**
 * @brief A tree widget useful for displaying a tree of folders containing messages
 *
 * This widget provides some facilities commonly used to display a tree of folders
 * containing messages. 
 *
 * Besides the standard functionality of a KPIM::TreeWidget this implementation will handle
 * four special columns: the "Label", the "Unread", the "Total" and the "DataSize" column.
 *
 * You add a Label column with addLabelColumn(). It will fetch the text data from
 * FolderTreeWidgetItem::labelText() instead of QTreeWidgetItem::text(). It will
 * also display the unread message count if the "Unread" column is not visible.
 *
 * You add a "Unread" column with addUnreadColumn(). It will fetch the numeric
 * count of unread messages from FolderTreeWidgetItem::unreadCount() and
 * it will automatically display it aligned to the right.
 *
 * You add a "Total" colum with addTotalColumn(). It will fetch the numeric
 * total count of messages from FolderTreeWidgetItem::totalCount() and it will
 * automatically display it aligned to the right.
 *
 * You add a "DataSize" column with addSizeColumn(). It will fetch the size in bytes
 * of the message folder from FolderTreeWidgetItem::size() and it will
 * display it aligned to the right and with a suitable textual rappresentation.
 *
 * You can set the color of the unread message count with setUnreadCountColor().
 * When the items are marked with the closeToQuota() warning then the text in the
 * "Label" colum is displayed with the color set by setCloseToQuotaWarningColor().
 *
 * @author: Szymon Tomasz Stefanek <pragma@kvirc.net>
 */
class KDEPIM_EXPORT FolderTreeWidget : public KPIM::TreeWidget
{
  friend class FolderTreeWidgetItemLabelColumnDelegate;
  Q_OBJECT
public:
  /**
   * Constructs a FolderTreeWidget instance.
   */
  explicit FolderTreeWidget( QWidget *parent , const char *name = 0 );

private:
  /**
   * Emits the renamed() signal
   */
  void emitRenamed( QTreeWidgetItem *item );
  int mLabelColumnIndex;      ///< The index for the special "Label" column or -1 if none
  int mUnreadColumnIndex;     ///< The index for the special "Unread" column or -1 if none
  int mTotalColumnIndex;      ///< The index for the special "Total" column or -1 if none
  int mDataSizeColumnIndex;   ///< The index for the special "DataSize" column or -1 if none

  QColor mUnreadCountColor;
  QColor mCloseToQuotaWarningColor;

public:
  /**
   * Adds a special "Label" column to this view and returns its logical index.
   * If a "Label" column was already present in the view then nothing is added
   * and the logical index of the existing column is returned.
   */
  int addLabelColumn( const QString &headerLabel );

  /**
   * Returns the logical index of the "Label" column or -1 if such a column
   * has not been added (yet).
   */
  int labelColumnIndex() const
    { return mLabelColumnIndex; };

  /**
   * Returns true if the widget contains a "Label" column and it's currently visible.
   */
  bool labelColumnVisible() const;

  /**
   * Adds a special "Unread" column to this view and returns its logical index.
   * If a "Unread" column was already present in the view then nothing is added
   * and the logical index of the existing column is returned.
   */
  int addUnreadColumn( const QString &headerLabel );

  /**
   * Returns the logical index of the "Unread" column or -1 if such a column
   * has not been added (yet).
   */
  int unreadColumnIndex() const
    { return mUnreadColumnIndex; };

  /**
   * Returns true if the widget contains an "Unread" column and it's currently visible.
   */
  bool unreadColumnVisible() const;

  /**
   * Adds a special "Total" column to this view and returns its logical index.
   * If a "Total" column was already present in the view then nothing is added
   * and the logical index of the existing column is returned.
   */
  int addTotalColumn( const QString &headerLabel );

  /**
   * Returns the logical index of the "Total" column or -1 if such a column
   * has not been added (yet).
   */
  int totalColumnIndex() const
    { return mTotalColumnIndex; };

  /**
   * Returns true if the widget contains a "Total" column and it's currently visible.
   */
  bool totalColumnVisible() const;

  /**
   * Adds a special "DataSize" column to this view and returns its logical index.
   * If a "DataSize" column was already present in the view then nothing is added
   * and the logical index of the existing column is returned.
   */
  int addDataSizeColumn( const QString &headerLabel );

  /**
   * Returns the logical index of the "DataSize" column or -1 if such a column
   * has not been added (yet).
   */
  int dataSizeColumnIndex() const
    { return mDataSizeColumnIndex; };

  /**
   * Returns true if the widget contains a "DataSize" column and it's currently visible.
   */
  bool dataSizeColumnVisible() const;

  /**
   * Returns the color used to display the "Label" column text when
   * the item is marked as close to quota.
   */
  const QColor & closeToQuotaWarningColor() const
    { return mCloseToQuotaWarningColor; };

  /**
   * Sets the color used to display the "Label" column text when
   * the item is marked as close to quota.
   */
  void setCloseToQuotaWarningColor( const QColor &clr );

  /**
   * Returns the color used to display the unread message count in the "Label" column.
   */
  const QColor & unreadCountColor() const
    { return mUnreadCountColor; };

  /**
   * Sets the color used to display the unread message count in the "Label" column.
   */
  void setUnreadCountColor( const QColor &clr );

  /**
   * Triggers an update for the specified column of the specified item.
   */
  void updateColumnForItem( FolderTreeWidgetItem * item, int columnIndex );

Q_SIGNALS:
  /**
   * This signal is emitted when the label of @p item  has changed
   * after an edition.
   */
  void renamed( QTreeWidgetItem *item );

private Q_SLOTS:

  /**
   * Connected to the itemExpanded/itemCollapsed signal, forwards this call
   * to updateExpandedState() of the item.
   */
  void updateExpandedState( QTreeWidgetItem *item );
};

class FolderTreeWidgetItemLabelColumnDelegate;

/**
 * @brief A folder tree node to be used with FolderTreeWidget
 *
 * @author Szymon Tomasz Stefanek <pragma@kvirc.net>
 */
class KDEPIM_EXPORT FolderTreeWidgetItem : public QTreeWidgetItem
{
  friend class FolderTreeWidgetItemLabelColumnDelegate;
public:
  /**
   * Protocol information associated to the item.
   * Please note that this list should be kept in the order of items
   * that one wants to be shown in the foldertreewidget.
   */
  enum Protocol {
      Local, Imap, CachedImap, News, Search, NONE
  };

  /**
   * Folder type information
   * Please note that this list should be kept in the order of items
   * that one wants to be shown in the FolderTreeWidget, but better
   * keep it in sync with the type in KFolderTree.
   */
  // FIXME: Rename to FolderItemType
  enum FolderType {
      Inbox, Outbox, SentMail, Trash, Drafts, Templates, Root,
      Calendar, Tasks, Journals, Contacts, Notes,  Other
  };

private:
  Protocol mProtocol;                  ///< The protocol associated to the folder
  FolderType mFolderType;              ///< The type of the folder
  QString mLabelText;                  ///< The text data for the special "Label" column
  int mTotalCount;                     ///< The total count of messages
  int mUnreadCount;                    ///< The count of unread messages
  qint64 mDataSize;                    ///< The size of the folder in bytes, -1 if meaningless
  unsigned int mIsCloseToQuota:1;      ///< The "Close to Quota" warning mark  
  unsigned int mLabelTextElided:1;     ///< The "Label text elided" warning mark
  int mChildrenTotalCount;             ///< The total count of messages in children
  int mChildrenUnreadCount;            ///< The total count of unread messages in children
  qint64 mChildrenDataSize;            ///< The size of the children folders in bytes, -1 if meaningless
  bool mAlwaysDisplayCounts;           ///< Bypasses data display logic for toplevel/children folders

public:
  /**
   * Constructs a root-item
   */
  explicit FolderTreeWidgetItem(
      FolderTreeWidget *parent,
      const QString &label,
      Protocol protocol,
      FolderType folderType
    );

  /**
   * Constructs a child-item
   */
  explicit FolderTreeWidgetItem(
      FolderTreeWidgetItem *parent,
      const QString &label,
      Protocol protocol,
      FolderType folderType
   );

public:
  /**
   * Returns the sate of the "Close to quota" warning for this folder.
   */
  bool isCloseToQuota() const
    { return mIsCloseToQuota; };

  /**
   * Sets the status of the close to quota warning.
   */
  void setIsCloseToQuota( bool closeToQuota );

  /**
   * Returns the textual data for the "Label" column of the parent FolderTreeWidget.
   */
  const QString & labelText() const
    { return mLabelText; };

  /**
   * Sets the textual data for the "Label" column of the parent FolderTreeWidget.
   */
  void setLabelText( const QString &label );

  /**
   * Returns the unread message count. Displayed in the special "Unread" column
   * by FolderTreeWidget.
   */
  int unreadCount() const
    { return mUnreadCount; };

  /**
   * Sets the unread message count to be displayed in the special "Unread" column.
   */
  void setUnreadCount( int unreadCount );

  /**
   * Returns the total message count. Displayed in the special "Total" column
   * by FolderTreeWidget.
   */
  int totalCount() const
    { return mTotalCount; };

  /**
   * Sets the total message count to be displayed in the special "Total" column.
   */
  void setTotalCount( int totalCount );

  /**
   * Returns the size in bytes of the folder, displayed in the special "DataSize" column.
   * If the size is meaningless (for folders that don't have storage, for example) then
   * this function returns -1.
   */
  qint64 dataSize() const
    { return mDataSize; };

  /**
   * Sets the size in bytes of the folder to be displayed in the special "DataSize" column.
   * If s is -1 then the size becomes meaningless and is displayed as a dash in the view.
   */
  void setDataSize( qint64 s );

  /**
   * Returns the unread message count for the children. Displayed in the
   * special "Unread" column by FolderTreeWidget.
   */
  int childrenUnreadCount() const
    { return mChildrenUnreadCount; };

  /**
   * Returns the total message count for the children. Displayed in the special
   * "Total" column by FolderTreeWidget.
   */
  int childrenTotalCount() const
    { return mChildrenTotalCount; };

  /**
   * Returns the size in bytes of the children folders
   * displayed in the special "DataSize" column.
   */
  qint64 childrenDataSize() const
    { return mChildrenDataSize; };

  /**
   * Gathers the counts for the children.
   * Please note that this function is NOT recursive. It will simply
   * scan the children and sum up their own+children counts.
   * It will also NOT update any text.
   * Returns true if any of the counts has actually changed.
   */
  bool updateChildrenCounts();

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
   * Returns a descriptive string of the folder's protocol
   */
  QString protocolDescription() const;

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

  /**
   * Triggers an update for the specified column of this item
   */
  void updateColumn( int columnIndex );

  /**
   * This should be called whenever this item is expanded/collapsed. It updates
   * the text for the total, unread and size column. If the item is collapsed,
   * that numbers will include the sum of the child numbers, otherwise it will
   * just be the numbers of this item.
   */
  void updateExpandedState();

  /**
   * Returns true if the last painting operation had to elide
   * the label text thus making it partially invisible.
   * This flag is meaningful only for currently visible items.
   */
  bool labelTextElided() const
    { return mLabelTextElided; };

  /**
   * Returns true if this item should bypass the size, total and unread
   * display logic. Normally when this item is a toplevel one and
   * has no children it doesn't display the counts. With this flag
   * set it displays the counts regardless of this.
   * FIXME: Why this logic is actually hardwired here ?
   */
  bool alwaysDisplayCounts() const
    { return mAlwaysDisplayCounts; };

  /**
   * Sets wheter this item should bypass the size, total and unread
   * display logic. Normally when this item is a toplevel one and
   * has no children it doesn't display the counts. With this flag
   * set it displays the counts regardless of this.
   * FIXME: Why this logic is actually hardwired here ?
   */
  void setAlwaysDisplayCounts( bool alwaysDisplayCounts )
    { mAlwaysDisplayCounts = alwaysDisplayCounts; };

protected:
  /**
   * This is called by FolderTreeWidgetItemLabelColumnDelegate
   * to update the mLabelTextElided flag.
   */
  void setLabelTextElided( bool labelTextElided )
    { mLabelTextElided = labelTextElided ? 1 : 0; };

  /**
   * Returns the label text that has been elided if necessary to
   * fit into the @p width for the given font @p metrics.
   * The default implementation elides the right of the label.
   */
  virtual QString elidedLabelText( const QFontMetrics &metrics, unsigned int width ) const;
};

} // namespace KPIM

#endif //!__FOLDERTREEWIDGET_H__
