#ifndef __TREEWIDGET_H__
#define __TREEWIDGET_H__

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
 * @file treewidget.h A common QTreeWidget extension
 */

#include "kdepim_export.h"

#include <QTreeWidget>

class KMenu;
class KConfig;
class KConfigGroup;

class QAction;


namespace KPIM
{

#define KPIM_TREEWIDGET_DEFAULT_CONFIG_KEY "TreeWidgetLayout"

/**
 * @brief A QTreeWidget with expanded capabilities
 * 
 * This class extends the functionality provided by the standard QTreeWidget
 * by adding toggleable columns and a layout save/restore facility.
 *
 * At the time of writing the usage of QTreeWidget is a bit against the
 * reccomended Qt4 approach. On the other hand in many cases you just want
 * a simple tree of items and don't want to care about anything complex like
 * a "data model".
 *
 * @author: Szymon Tomasz Stefanek <pragma@kvirc.net>
 */
class KDEPIM_EXPORT TreeWidget : public QTreeWidget
{
  Q_OBJECT
public:
  /**
   * Constructs a TreeWidget. Sets up the default values
   * for properties.
   */
  explicit TreeWidget( QWidget *parent, const char *name = 0 );

private:
  bool mEnableManualColumnHiding; ///< Is manual column hiding currently enabled ?

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
      const char *keyName = KPIM_TREEWIDGET_DEFAULT_CONFIG_KEY
    ) const;

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
      const char *keyName = KPIM_TREEWIDGET_DEFAULT_CONFIG_KEY
    ) const;

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
   * @returns true if the layout data has been successfully read and
   *          restored. Returns false if the specified key of the config
   *          group is empty, or the data contained inside is not valid.
   */
  bool restoreLayout(
      KConfigGroup &group,
      const char *keyName = KPIM_TREEWIDGET_DEFAULT_CONFIG_KEY            
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
   * @returns true if the layout data has been successfully read and
   *          restored. Returns false if the specified key of the config
   *          group is empty, or the data contained inside is not valid.
   */
  bool restoreLayout(
      KConfig *config,
      const char *groupName,
      const char *keyName = KPIM_TREEWIDGET_DEFAULT_CONFIG_KEY
    );

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
   * The alignment flags refer to the header text only. For individual
   * items you must set the alignment manually.
   *
   * @param label The label for the column
   * @param alignment The alignment of the header text for this column
   */
  int addColumn(
      const QString &label ,
      int headerAlignment = Qt::AlignLeft | Qt::AlignVCenter
    );

  /**
   * Convenience function that changes the header text for the specified column.
   * Returns true if the text can be successfully changed or false
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

} // namespace KPIM

#endif //!__TREEWIDGET_H__
