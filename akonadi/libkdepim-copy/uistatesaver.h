/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef UISTATESAVER_H
#define UISTATESAVER_H

#include "libkdepim-copy_export.h"

class QWidget;
class KConfigGroup;

namespace KPIM {

/**
 * Methods to save and restore the UI state of an application.
 * The following widgets are supported so far:
 * - QSplitter
 * - QTabWidget
 * - QTreeView
 * - QComboBox
 */
namespace UiStateSaver {

  /**
   * Save the state of @p widget and all its sub-widgets
   * to @p config.
   * @param widget The top-level widget which state should
   * be saved.
   * @param config The config group the settings should be
   * written to.
   */
  KDEPIM_COPY_EXPORT void saveState( QWidget* widget, KConfigGroup &config );

  /**
   * Restore UI state of @p widget and all its sub-widgets
   * from @p config.
   * @param widget The top-level widget which state should
   * be restored.
   * @param config The config gorup the settings should be
   * read from.
   */
  KDEPIM_COPY_EXPORT void restoreState( QWidget *widget, const KConfigGroup &config );
}

}

#endif
