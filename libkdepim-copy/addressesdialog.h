/*  -*- mode: C++; c-file-style: "gnu" -*-
 *
 *  This file is part of libkdepim.
 *
 *  Copyright (c) 2003 Zack Rusin <zack@kde.org>
 *  Copyright (c) 2003 Aaron J. Seigo <aseigo@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef KDEPIM_ADDRESSESDIALOG_H
#define KDEPIM_ADDRESSESDIALOG_H

#include "kdepim_export.h"
#include "ui_addresspicker.h"

#include <kabc/addressee.h>

#include <KDialog>
#include <K3ListView>

#include <QStringList>
#include <Q3PtrList>
#include <Q3PtrDict>

class AddressPickerUI : public QWidget, public Ui::AddressPickerUI
{
public:
  AddressPickerUI( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};



namespace KPIM {

  class AddresseeViewItem : public QObject, public K3ListViewItem
  {
  Q_OBJECT

  public:
    enum Category {
      To          =0,
      CC          =1,
      BCC         =2,
      Group       =3,
      Entry       =4,
      FilledGroup =5,
      DistList    =6
    };
    AddresseeViewItem( AddresseeViewItem *parent, const KABC::Addressee& addr, int emailIndex = 0 );
    AddresseeViewItem( K3ListView *lv, const QString& name, Category cat=Group );
    AddresseeViewItem( AddresseeViewItem *parent, const QString& name, const KABC::Addressee::List &lst );
    AddresseeViewItem( AddresseeViewItem *parent, const QString& name );
    ~AddresseeViewItem();

    KABC::Addressee       addressee() const;
    KABC::Addressee::List addresses() const;
    Category              category() const;

    QString name()  const;
    QString email() const;

    bool matches( const QString& ) const;

    virtual int compare( Q3ListViewItem * i, int col, bool ascending ) const;
    virtual void setSelected( bool );

  Q_SIGNALS:
    void addressSelected( AddresseeViewItem*, bool );

  private:
    struct AddresseeViewItemPrivate;
    AddresseeViewItemPrivate *d;
  };

  class KDEPIM_EXPORT AddressesDialog : public KDialog
  {
    Q_OBJECT
  public:
    AddressesDialog( QWidget *widget=0 );
    ~AddressesDialog();

    /**
     * Returns the list of picked "To" addresses as a QStringList.
     */
    QStringList to()  const;
    /**
     * Returns the list of picked "CC" addresses as a QStringList.
     */
    QStringList cc()  const;
    /**
     * Returns the list of picked "BCC" addresses as a QStringList.
     */
    QStringList bcc() const;

    /**
     * Returns the list of picked "To" addresses as KABC::Addressee::List.
     * Note that this doesn't include the distribution lists
     */
    KABC::Addressee::List toAddresses()  const;
   /**
     * Returns the list of picked "To" addresses as KABC::Addressee::List.
     * Note that this does include the distribution lists
     * Multiple Addressees are removed
     */
    KABC::Addressee::List allToAddressesNoDuplicates()  const;
    /**
     * Returns the list of picked "CC" addresses as KABC::Addressee::List.
     * Note that this doesn't include the distribution lists
     */
    KABC::Addressee::List ccAddresses()  const;
    /**
     * Returns the list of picked "BCC" addresses as KABC::Addressee::List.
     * Note that this doesn't include the distribution lists
     */
    KABC::Addressee::List bccAddresses() const;

    /**
     * Returns the list of picked "To" distribution lists.
     * This complements @ref toAddresses.
     */
    QStringList toDistributionLists() const;
    /**
     * Returns the list of picked "CC" distribution lists.
     * This complements @ref ccAddresses.
     */
    QStringList ccDistributionLists() const;
    /**
     * Returns the list of picked "BCC" distribution lists.
     * This complements @ref bccAddresses.
     */
    QStringList bccDistributionLists() const;

  public Q_SLOTS:
    /**
     * Displays the CC field if @p b is true, else
     * hides it. By default displays it.
     */
    void setShowCC( bool b );
    /**
     * Displays the BCC field if @p b is true, else
     * hides it. By default displays it.
     */
    void setShowBCC( bool b );
    /**
     * If called adds "Recent Addresses" item to the picker list view,
     * with the addresses given in @p addr.
     */
    void setRecentAddresses( const KABC::Addressee::List& addr );
    /**
     * Adds addresses in @p l to the selected "To" group.
     */
    void setSelectedTo( const QStringList& l );
     /**
     * Adds addresses in @p l to the selected "CC" group.
     */
    void setSelectedCC( const QStringList& l );
     /**
     * Adds addresses in @p l to the selected "BCC" group.
     */
    void setSelectedBCC( const QStringList& l );

  protected Q_SLOTS:
    void addSelectedTo();
    void addSelectedCC();
    void addSelectedBCC();

    void removeEntry();
    void saveAs();
    void searchLdap();
    void ldapSearchResult();
    void launchAddressBook();

    void filterChanged( const QString & );

    void updateAvailableAddressees();
    void availableSelectionChanged();
    void selectedSelectionChanged();
    void availableAddressSelected( AddresseeViewItem* item, bool selected );
    void selectedAddressSelected( AddresseeViewItem* item, bool selected );

  protected:
    AddresseeViewItem* selectedToItem();
    AddresseeViewItem* selectedCcItem();
    AddresseeViewItem* selectedBccItem();

    void initConnections();
    void addDistributionLists();
    void addAddresseeToAvailable( const KABC::Addressee& addr,
                                  AddresseeViewItem* defaultParent=0, bool useCategory=true );
    void addAddresseeToSelected( const KABC::Addressee& addr,
                                 AddresseeViewItem* defaultParent=0 );
    void addAddresseesToSelected( AddresseeViewItem *parent,
                                  const Q3PtrList<AddresseeViewItem>& addresses );
    QStringList entryToString( const KABC::Addressee::List& l ) const;
    KABC::Addressee::List allAddressee( AddresseeViewItem* parent ) const;
    KABC::Addressee::List allAddressee( K3ListView* view, bool onlySelected = true ) const;
    QStringList allDistributionLists( AddresseeViewItem* parent ) const;

  private:
    // if there's only one group in the available list, open it
    void checkForSingleAvailableGroup();

    // used to re-show items in the available list
    // it is recursive, but should only ever recurse once so should be fine
    void unmapSelectedAddress(AddresseeViewItem* item);
    void updateRecentAddresses();

    struct AddressesDialogPrivate;
    AddressesDialogPrivate *d;

    Q3PtrList<AddresseeViewItem> selectedAvailableAddresses;
    Q3PtrList<AddresseeViewItem> selectedSelectedAddresses;
    Q3PtrDict<AddresseeViewItem> selectedToAvailableMapping;
  };

}

#endif /* ADDRESSESDIALOG_H */
