/*
    This file is part of libkdepim.

    Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef KPIM_ADDRESSEESELECTOR_H
#define KPIM_ADDRESSEESELECTOR_H

#include <kabc/addressee.h>
#include <kabc/distributionlist.h>
#include <kabc/resource.h>
#include <kdialogbase.h>

#include <qbitarray.h>
#include <qpixmap.h>
#include <qwidget.h>

class KComboBox;
class KLineEdit;
class KListView;
class QSignalMapper;

namespace KPIM {

class Selection
{
  friend class AddresseeSelector;

  public:
    virtual ~Selection() {}

    /**
      Returns the number of fields the selection offers.
     */
    virtual uint fieldCount() const = 0;

    /**
      Returns the title for the field specified by index.
     */
    virtual QString fieldTitle( uint index ) const = 0;

    /**
      Returns the number of items for the given addressee.
     */
    virtual uint itemCount( const KABC::Addressee &addresse ) const = 0;

    /**
      Returns the text that's used for the item specified by index.
     */
    virtual QString itemText( const KABC::Addressee &addresse, uint index ) const = 0;

    /**
      Returns the icon that's used for the item specified by index.
     */
    virtual QPixmap itemIcon( const KABC::Addressee &addresse, uint index ) const = 0;

    /**
      Returns whether the item specified by index is enabled.
     */
    virtual bool itemEnabled( const KABC::Addressee &addresse, uint index ) const = 0;

    /**
      Returns whether the item specified by index matches the passed pattern.
     */
    virtual bool itemMatches( const KABC::Addressee &addresse, uint index, const QString &pattern ) const = 0;

    /**
      Returns whether the item specified by index equals the passed pattern.
     */
    virtual bool itemEquals( const KABC::Addressee &addresse, uint index, const QString &pattern ) const = 0;

    /**
      Returns the text that's used for the given distribution list.
     */
    virtual QString distributionListText( const KABC::DistributionList *distributionList ) const = 0;

    /**
      Returns the icon that's used for the given distribution list.
     */
    virtual QPixmap distributionListIcon( const KABC::DistributionList *distributionList ) const = 0;

    /**
      Returns whether the given distribution list is enabled.
     */
    virtual bool distributionListEnabled( const KABC::DistributionList *distributionList ) const = 0;

    /**
      Returns whether the given distribution list matches the passed pattern.
     */
    virtual bool distributionListMatches(  const KABC::DistributionList *distributionList,
                                           const QString &pattern ) const = 0;

    /**
      Returns the number of additional address books.
     */
    virtual uint addressBookCount() const = 0;

    /**
      Returns the title for an additional address book.
     */
    virtual QString addressBookTitle( uint index ) const = 0;

    /**
      Returns the content for an additional address book.
     */
    virtual KABC::Addressee::List addressBookContent( uint index ) const = 0;

  protected:
    AddresseeSelector* selector() { return mSelector; }

  private:
    virtual void addSelectedAddressees( uint fieldIndex, const KABC::Addressee&, uint itemIndex ) = 0;
    virtual void addSelectedDistributionList( uint fieldIndex, const KABC::DistributionList* ) = 0;

    void setSelector( AddresseeSelector *selector ) { mSelector = selector; }

    AddresseeSelector *mSelector;
};

/**
  Internal heper class
 */
class SelectionItem
{
  public:
    typedef QValueList<SelectionItem> List;

    SelectionItem( const KABC::Addressee &addressee, uint index );
    SelectionItem( KABC::DistributionList *list, uint index );
    SelectionItem();

    void addToField( int index );
    void removeFromField( int index );
    bool isInField( int index );

    KABC::Addressee addressee() const;
    KABC::DistributionList* distributionList() const;
    uint index() const;

  private:
    KABC::Addressee mAddressee;
    KABC::DistributionList *mDistributionList;
    uint mIndex;
    QBitArray mField;
};

class AddresseeSelector : public QWidget
{
  Q_OBJECT

  public:
    AddresseeSelector( Selection *selection,
                       QWidget *parent, const char *name = 0 );
    ~AddresseeSelector();

    /**
      Writes back the selected items to the selection.
     */
    void finish();

    void setItemSelected( uint fieldIndex, const KABC::Addressee&, uint itemIndex );
    void setItemSelected( uint fieldIndex, const KABC::Addressee&,
                          uint itemIndex, const QString& );

  private slots:
    void move( int index );
    void remove( int index );

    void updateAddresseeView();
    void reloadAddressBook();

  private:
    void init();
    void initGUI();

    void updateSelectionView( int index );

    Selection *mSelection;

    KComboBox *mAddressBookCombo;
    KLineEdit *mAddresseeFilter;
    KListView *mAddresseeView;
    SelectionItem::List mSelectionItems;

    QValueList<KListView*> mSelectionViews;
    QSignalMapper *mMoveMapper;
    QSignalMapper *mRemoveMapper;

    KABC::DistributionListManager *mManager;

    class AddressBookManager;
    AddressBookManager *mAddressBookManager;
};

class AddresseeSelectorDialog : public KDialogBase
{
  Q_OBJECT

  public:
    AddresseeSelectorDialog( Selection *selection,
                             QWidget *parent = 0, const char *name = 0 );

  protected slots:
    void accept();

  private:
    AddresseeSelector *mSelector;
};

}

#endif
