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

#include <QTreeWidget>

class KMenu;
class KConfig;
class KConfigGroup;

class QAction;


namespace KPIM
{

#define FTWB_DEFAULT_CONFIG_KEY "TreeWidgetLayout"

/**
 * @brief A QTreeWidget with expanded capabilities
 * 
 * This class extends the functionality provided by the standard QTreeWidget
 * by adding toggleable columns and a layout save/restore facility.
 *
 * @author: Szymon Tomasz Stefanek <pragma@kvirc.net>
 */
class KDEPIM_EXPORT FolderTreeWidgetBase : public QTreeWidget
{
  Q_OBJECT
public:
  /**
   * Constructs a FolderTreeWidgetBase. Sets up the default values
   * for properties.
   */
  explicit FolderTreeWidgetBase( QWidget *parent, const char *name = 0 );

private:
  bool mEnableManualColumnHiding; //< Is manual column hiding currently enabled ?

public:

  //
  // Layout Save/Restore facility
  //

  /**
   * Stores the layout of this tree view to the specified KConfigGroup.
   * The stored data includes visible columns, column width and order.
   *
   * @param group The KConfigGroup to write the layout data to.
   * @param keyName The key to use, "TreeWidgetLayout" by default
   *
   * @returns true on success and false on failure (if keyName is 0, for example).
   */
  bool saveLayout(
      KConfigGroup &group,
      const char *keyName = FTWB_DEFAULT_CONFIG_KEY
    );

  /**
   * Stores the layout of this tree view to the specified key of 
   * the specified config group.
   * The stored data includes visible columns, column width and order.
   * 
   * @param configGroup The KConfig group to write the layout data to.
   * @param keyName The key to use, "TreeWidgetLayout" by default
   *
   * @returns true on success and false on failure (if you can't write
   * to the configGroup).
   */
  bool saveLayout(
      KConfig *config,
      const char *groupName,
      const char *keyName = FTWB_DEFAULT_CONFIG_KEY
    );

  /**
   * Attempts to restore the layout of this tree from the specified
   * key of the specified KConfigGroup. The layout must have been
   * previously saved by a call to saveLayout().
   *
   * Please note that you must call this function after having added
   * all the relevant columns to the view. On the other hand if you're
   * setting some of the view properties programmatically (like last section
   * stretching) then you should set them after restoring the layout
   * because otherwise they might get overwritten by the stored data.
   * The proper order is: add columns, restore layout, set header properties.
   *
   * @param group The KConfigGroup to read the layout data from.
   * @param keyName The key to use, "TreeWidgetLayout" by default
   *
   * @returns true if the layout data has been succesfully read and
   *          restored. Returns false if the specified key of the config
   *          group is empty, or the data contained inside is not valid.
   */
  bool restoreLayout(
      KConfigGroup &group,
      const char *keyName = FTWB_DEFAULT_CONFIG_KEY            
    );

  /**
   * Attempts to restore the layout of this tree from the specified
   * key of the specified group. The layout must have been
   * previously saved by a call to saveLayout().
   *
   * Please note that you must call this function after having added
   * all the relevant columns to the view. On the other hand if you're
   * setting some of the view properties programmatically (like last section
   * stretching) then you should set them after restoring the layout
   * because otherwise they might get overwritten by the stored data.
   * The proper order is: add columns, restore layout, set header properties.
   *
   * @param configGroup The name of the KConfig group to read the layout data from.
   * @param keyName The key to use, "TreeWidgetLayout" by default
   *
   * @returns true if the layout data has been succesfully read and
   *          restored. Returns false if the specified key of the config
   *          group is empty, or the data contained inside is not valid.
   */
  bool restoreLayout( KConfig *config, const char *groupName, const char *keyName = FTWB_DEFAULT_CONFIG_KEY );

  //
  // Manual and programmatic column hiding
  //

  /**
   * Enables or disables manual column hiding by the means of a contextual menu.
   * All columns but the first can be hidden by the user by toggling the
   * check on the popup menu that is displayed by right clicking the header
   * of this view.
   *
   * By default column hiding is enabled. 
   */
  void setManualColumnHidingEnabled( bool enable );

  /**
   * @returns true if manual column hiding is currently enabled, false otherwise.
   */
  bool manualColumnHidingEnabled() const
    { return mEnableManualColumnHiding; };

  /**
   * Hides or shows the column specified by logicalIndex.
   * The column is hidden if hide is true and shown otherwise.
   */
  void setColumnHidden( int logicalIndex, bool hide );

  /**
   * Returns true if the column specified by logicalIndex is actually hidden.
   */
  bool isColumnHidden( int logicalIndex ) const;

  /**
   * Called by this class to fill the contextual menu shown when the
   * user right clicks on one of the header columns and column hiding
   * is enabled.
   *
   * If you override this, remember to call the base class implementation
   * if you want to keep column hiding working.
   *
   * You should return true if you want the menu to be shown or false
   * if you don't want it to be shown. The default implementation returns
   * true unless you have no columns at all, column hiding is disabled
   * or you pass some weird parameter.
   */
  virtual bool fillHeaderContextMenu( KMenu * menu , const QPoint &clickPoint );

  //
  // Facilities for adding and manipulating columns
  //

  /**
   * Convenience function that adds a column to this tree widget
   * and returns its logical index. Please note that in fact
   * this function manipulates the header item which in turn
   * plays with the underlying data model.
   *
   * Anyway.. this is what you want to use 99,9% of the times :)
   */
  int addColumn( const QString &label );

  /**
   * Convenience function that changes the text of the specified column.
   * Returns true if the text can be succesfully changed or false
   * if the specified column index is out of range.
   */
  bool setColumnText( int columnIndex , const QString &label );

protected Q_SLOTS:
  /**
   * Internal slot connected to the customContextMenuRequested()
   * signal of the header().
   */
  void slotHeaderContextMenuRequested( const QPoint &clickPoint );

  /**
   * Internal slot connected to the show/hide actions in the
   * header contextual menu.
   */
  void slotToggleColumnActionTriggered( QAction *act );

};


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
class KDEPIM_EXPORT FolderTreeWidget : public FolderTreeWidgetBase
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
  Protocol mProtocol;         //< The protocol associated to the folder
  FolderType mFolderType;     //< The type of the folder

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
