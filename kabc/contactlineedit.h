/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef CONTACTLINEEDIT_H
#define CONTACTLINEEDIT_H

#include "akonadi-kabccommon_export.h"

#include <QtGui/QLineEdit>

class QCompleter;

namespace Akonadi {

class Item;
class KABCModel;

/**
 * @short A line edit that has auto completion for a contact/contact group model.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.3
 */
class AKONADI_KABCCOMMON_EXPORT ContactLineEdit : public QLineEdit
{
  public:
    /**
     * Creates a new contact line edit.
     *
     * @param model The contact/contact group model the line edit uses for completion.
     * @param parent The parent widget.
     */
    ContactLineEdit( KABCModel *model, QWidget *parent = 0 );

    /**
     * Returns the item the user typed in/selected or an invalid item
     * if there is no such item available.
     */
    Item currentItem() const;

  private:
    QCompleter *mCompleter;
    KABCModel *mModel;
};

}

#endif
