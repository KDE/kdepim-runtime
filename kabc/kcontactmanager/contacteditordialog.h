/*
    This file is part of KContactManager.

    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef CONTACTEDITORDIALOG_H
#define CONTACTEDITORDIALOG_H

#include <kdialog.h>

#include <akonadi/item.h>

class KABCItemEditor;

class ContactEditorDialog : public KDialog
{
  Q_OBJECT

  public:
    enum Mode
    {
      CreateMode, ///< Creates a new contact
      EditMode    ///< Edits an existing contact
    };

    /**
     * Creates a new contact editor dialog.
     *
     * @param mode The mode of the dialog.
     * @param collectionModel The collection model.
     * @param parent The parent widget of the dialog.
     */
    ContactEditorDialog( Mode mode, QAbstractItemModel *collectionModel, QWidget *parent = 0 );

    /**
     * Destroys the contact editor dialog.
     */
    ~ContactEditorDialog();

    /**
     * Sets the @p contact to edit when in EditMode.
     */
    void setContact( const Akonadi::Item &contact );

  Q_SIGNALS:
    /**
     * This signal is emitted whenever a contact was updated or stored.
     *
     * @param contact The data reference of the contact.
     */
    void contactStored( const Akonadi::Item &contact );

  private Q_SLOTS:
    void slotOkClicked();
    void slotCancelClicked();

  private:
    KABCItemEditor *mEditor;    
};

#endif
