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

#include "addressesdialog.h"
#include "addresspicker.h"

#include <config.h>

#ifdef KDEPIM_NEW_DISTRLISTS
#include "distributionlist.h"
#include <kresources/selectdialog.h>
#include <kabc/resource.h>
#else
#include <kabc/distributionlist.h>
#endif

#include <kabc/stdaddressbook.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kpushbutton.h>
#include <krun.h>
#include <kstandarddirs.h>

#include <q3dict.h>
#include <QLayout>
#include <kvbox.h>
#include <QWidget>
//Added by qt3to4:
#include <Q3PtrList>
//Added by qt3to4:
#include <QPixmap>
#include <ktoolinvocation.h>

namespace KPIM {

// private start :
struct AddresseeViewItem::AddresseeViewItemPrivate {
  KABC::Addressee               address;
  AddresseeViewItem::Category   category;
  KABC::Addressee::List         addresses;
};

struct AddressesDialog::AddressesDialogPrivate {
  AddressesDialogPrivate() : ui(0), personal(0), recent(0), toItem(0), ccItem(0), bccItem(0)
  {}

  AddressPickerUI             *ui;

  AddresseeViewItem           *personal;
  AddresseeViewItem           *recent;

  AddresseeViewItem           *toItem;
  AddresseeViewItem           *ccItem;
  AddresseeViewItem           *bccItem;

  Q3Dict<AddresseeViewItem>     groupDict;

  KABC::Addressee::List       recentAddresses;
};
// privates end

AddresseeViewItem::AddresseeViewItem( AddresseeViewItem *parent, const KABC::Addressee& addr,
                                      int emailIndex )
  : QObject( 0 ), K3ListViewItem( parent, addr.realName(),
                               ( emailIndex == 0 ? addr.preferredEmail() : addr.emails()[ emailIndex ] ) )
{
  d = new AddresseeViewItemPrivate;
  d->address = addr;
  d->category = Entry;

  if ( text( 0 ).trimmed().isEmpty() )
    setText( 0, addr.preferredEmail() );

  if ( addr.photo().url().isEmpty() ) {
    if ( addr.photo().data().isNull() )
      setPixmap( 0, KGlobal::iconLoader()->loadIcon( "personal", K3Icon::Small ) );
    else
      setPixmap( 0, QPixmap::fromImage( addr.photo().data().scaled( 16, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation
) ) );
  } else {
    setPixmap( 0, KGlobal::iconLoader()->loadIcon( addr.photo().url(), K3Icon::Small ) );
  }
}

AddresseeViewItem::AddresseeViewItem( K3ListView *lv, const QString& name, Category cat )
  : QObject(0), K3ListViewItem( lv, name )
{
  d = new AddresseeViewItemPrivate;
  d->category = cat;
}

AddresseeViewItem::AddresseeViewItem(  AddresseeViewItem *parent, const QString& name,
                                       const KABC::Addressee::List &lst )
  : QObject(0), K3ListViewItem( parent, name, i18n("<group>") )
{
  d = new AddresseeViewItemPrivate;
  d->category = FilledGroup;
  d->addresses = lst;
}

AddresseeViewItem::AddresseeViewItem(  AddresseeViewItem *parent, const QString& name )
  : QObject(0), K3ListViewItem( parent, name, i18n("<group>") )
{
  d = new AddresseeViewItemPrivate;
  d->category = DistList;

  setPixmap( 0, KGlobal::iconLoader()->loadIcon( "kdmconfig", K3Icon::Small ) );
}

AddresseeViewItem::~AddresseeViewItem()
{
  delete d;
  d = 0;
}

KABC::Addressee
AddresseeViewItem::addressee() const
{
  return d->address;
}

KABC::Addressee::List
AddresseeViewItem::addresses() const
{
  return d->addresses;
}

AddresseeViewItem::Category
AddresseeViewItem::category() const
{
  return d->category;
}

QString
AddresseeViewItem::name() const
{
  return text(0);
}

QString
AddresseeViewItem::email() const
{
  return text(1);
}

bool AddresseeViewItem::matches(const QString& txt) const
{
    return d->address.realName().contains(txt, Qt::CaseInsensitive)
        || d->address.preferredEmail().contains(txt, Qt::CaseInsensitive);
}

void AddresseeViewItem::setSelected(bool selected)
{
    if (selected == isSelected())
    {
        return;
    }

    emit addressSelected( this, selected );
    Q3ListViewItem::setSelected(selected);
}

int
AddresseeViewItem::compare( Q3ListViewItem * i, int col, bool ascending ) const
{
  if ( category() == Group || category() == Entry )
    return K3ListViewItem::compare( i , col, ascending );

  AddresseeViewItem *item = static_cast<AddresseeViewItem*>( i );
  int a = static_cast<int>( category() );
  int b = static_cast<int>( item->category() );

  if ( ascending )
    if ( a < b )
      return -1;
    else
      return 1;
  else
    if ( a < b )
      return 1;
    else
      return -1;
}

AddressesDialog::AddressesDialog( QWidget* parent)
  : KDialog( parent )
{
  setCaption( i18n( "Address Selection" ) );
  setButtons( Ok|Cancel );
  setDefaultButton( Ok );
  setModal( true );
  KVBox *page = new KVBox( this );
  setMainWidget( page );
  d = new AddressesDialogPrivate;
  d->ui = new AddressPickerUI( page );

  KABC::StdAddressBook::self( true );
  updateAvailableAddressees();
  initConnections();

  d->ui->mAvailableView->setFocus();
}

AddressesDialog::~AddressesDialog()
{
  delete d;
  d = 0;
}

AddresseeViewItem* AddressesDialog::selectedToItem()
{
  if ( !d->toItem )
  {
    d->toItem = new AddresseeViewItem( d->ui->mSelectedView, i18n("To"), AddresseeViewItem::To );
    connect(d->toItem, SIGNAL(addressSelected(AddresseeViewItem*, bool)),
            this, SLOT(selectedAddressSelected(AddresseeViewItem*, bool)));
  }
  return d->toItem;
}

AddresseeViewItem* AddressesDialog::selectedCcItem()
{
  if ( !d->ccItem )
  {
    d->ccItem = new AddresseeViewItem( d->ui->mSelectedView, i18n("CC"), AddresseeViewItem::CC );
    connect(d->ccItem, SIGNAL(addressSelected(AddresseeViewItem*, bool)),
            this, SLOT(selectedAddressSelected(AddresseeViewItem*, bool)));
  }
  return d->ccItem;
}

AddresseeViewItem* AddressesDialog::selectedBccItem()
{
  if ( !d->bccItem )
  {
    d->bccItem = new AddresseeViewItem( d->ui->mSelectedView, i18n("BCC"), AddresseeViewItem::BCC );
    connect(d->bccItem, SIGNAL(addressSelected(AddresseeViewItem*, bool)),
            this, SLOT(selectedAddressSelected(AddresseeViewItem*, bool)));
  }
  return d->bccItem;
}

void
AddressesDialog::setSelectedTo( const QStringList& l )
{
  QString name, email;
  for ( QStringList::ConstIterator it = l.begin(); it != l.end(); ++it ) {
    KABC::Addressee addr;
    KABC::Addressee::parseEmailAddress( *it, name, email );
    addr.setNameFromString( name );
    addr.insertEmail( email );
    addAddresseeToSelected( addr, selectedToItem() );
  }
}

void
AddressesDialog::setSelectedCC( const QStringList& l )
{
  QString name, email;
  for ( QStringList::ConstIterator it = l.begin(); it != l.end(); ++it ) {
    KABC::Addressee addr;
    KABC::Addressee::parseEmailAddress( *it, name, email );
    addr.setNameFromString( name );
    addr.insertEmail( email );
    addAddresseeToSelected( addr, selectedCcItem() );
  }
}

void
AddressesDialog::setSelectedBCC( const QStringList& l )
{
  QString name, email;
  for ( QStringList::ConstIterator it = l.begin(); it != l.end(); ++it ) {
    KABC::Addressee addr;
    KABC::Addressee::parseEmailAddress( *it, name, email );
    addr.setNameFromString( name );
    addr.insertEmail( email );
    addAddresseeToSelected( addr, selectedBccItem() );
  }
}

void
AddressesDialog::setRecentAddresses( const KABC::Addressee::List& list )
{
  d->recentAddresses = list;

  updateRecentAddresses();

  checkForSingleAvailableGroup();
}

void
AddressesDialog::updateRecentAddresses()
{
  static const QString &recentGroup = KGlobal::staticQString( i18n( "Recent Addresses" ) );

  if ( !d->recent ) {
    d->recent = new AddresseeViewItem( d->ui->mAvailableView, recentGroup );
    connect(d->recent, SIGNAL(addressSelected(AddresseeViewItem*, bool)),
            this, SLOT(availableAddressSelected(AddresseeViewItem*, bool)));
    d->recent->setVisible( false );
    d->groupDict.insert( recentGroup, d->recent );
  }

  KABC::Addressee::List::ConstIterator it;
  for ( it = d->recentAddresses.begin(); it != d->recentAddresses.end(); ++it )
    addAddresseeToAvailable( *it, d->recent );

  if ( d->recent->childCount() > 0 ) {
    d->recent->setVisible( true );
  }
}

void
AddressesDialog::setShowCC( bool b )
{
  d->ui->mCCButton->setVisible( b );
}

void
AddressesDialog::setShowBCC( bool b )
{
  d->ui->mBCCButton->setVisible( b );
}

QStringList
AddressesDialog::to() const
{
  QStringList emails = allDistributionLists( d->toItem );
  KABC::Addressee::List l = toAddresses();
  emails += entryToString( l );

  return emails;
}

QStringList
AddressesDialog::cc() const
{
  QStringList emails = allDistributionLists( d->ccItem );
  KABC::Addressee::List l = ccAddresses();
  emails += entryToString( l );

  return emails;
}

QStringList
AddressesDialog::bcc() const
{
  QStringList emails = allDistributionLists( d->bccItem );

  KABC::Addressee::List l = bccAddresses();
  emails += entryToString( l );

  return emails;
}

KABC::Addressee::List
AddressesDialog::toAddresses()  const
{
  return allAddressee( d->toItem );
}
KABC::Addressee::List
AddressesDialog::allToAddressesNoDuplicates()  const
{
  KABC::Addressee::List aList = allAddressee( d->toItem );
  const QStringList dList = toDistributionLists();
  KABC::AddressBook* abook = KABC::StdAddressBook::self( true );
#ifdef KDEPIM_NEW_DISTRLISTS
  for ( QStringList::ConstIterator it = dList.begin(); it != dList.end(); ++it ) {
    const QList<KPIM::DistributionList::Entry> eList
      = KPIM::DistributionList::findByName(abook, *it).entries(abook);
    QList<KPIM::DistributionList::Entry>::ConstIterator eit;
    for( eit = eList.begin(); eit != eList.end(); ++eit ) {
      KABC::Addressee a = (*eit).addressee;
      if ( !a.preferredEmail().isEmpty() && !aList.contains( a ) ) {
          aList.append( a ) ;
      }
    }
  }
#else
  KABC::DistributionListManager manager( abook );
  manager.load();
  for ( QStringList::ConstIterator it = dList.begin(); it != dList.end(); ++it ) {
    const QList<KABC::DistributionList::Entry> eList = manager.list( *it )->entries();
    QList<KABC::DistributionList::Entry>::ConstIterator eit;
    for( eit = eList.begin(); eit != eList.end(); ++eit ) {
      KABC::Addressee a = (*eit).addressee;
      if ( !a.preferredEmail().isEmpty() && !aList.contains( a ) ) {
          aList.append( a ) ;
      }
    }
  }
#endif
  return aList;
}

KABC::Addressee::List
AddressesDialog::ccAddresses()  const
{
  return allAddressee( d->ccItem );
}

KABC::Addressee::List
AddressesDialog::bccAddresses()  const
{
  return allAddressee( d->bccItem );
}


QStringList
AddressesDialog::toDistributionLists() const
{
  return allDistributionLists( d->toItem );
}

QStringList
AddressesDialog::ccDistributionLists() const
{
  return allDistributionLists( d->ccItem );
}

QStringList
AddressesDialog::bccDistributionLists() const
{
  return allDistributionLists( d->bccItem );
}

void
AddressesDialog::updateAvailableAddressees()
{
  d->ui->mAvailableView->clear();
  d->groupDict.clear();

  static const QString &personalGroup = KGlobal::staticQString( i18n( "Other Addresses" ) );
  d->ui->mAvailableView->setRootIsDecorated( true );
  d->personal = new AddresseeViewItem( d->ui->mAvailableView, personalGroup );
  //connect(d->personal, SIGNAL(addressSelected(AddresseeViewItem*, bool)),
  //        this, SLOT(selectedAddressSelected(AddresseeViewItem*, bool)));
  d->personal->setVisible( false );
  d->groupDict.insert( personalGroup, d->personal );

  KABC::AddressBook *addressBook = KABC::StdAddressBook::self( true );
  for( KABC::AddressBook::Iterator it = addressBook->begin();
       it != addressBook->end(); ++it ) {
    addAddresseeToAvailable( *it, d->personal );
  }

  d->recent = 0;
  updateRecentAddresses();

  addDistributionLists();
  if ( d->personal->childCount() > 0 ) {
    d->personal->setVisible( true );
  }

  checkForSingleAvailableGroup();
}

void AddressesDialog::checkForSingleAvailableGroup()
{
  Q3ListViewItem* item = d->ui->mAvailableView->firstChild();
  Q3ListViewItem* firstGroup = 0;
  int found = 0;
  while (item)
  {
    if (item->isVisible())
    {
      if (!firstGroup && static_cast<AddresseeViewItem*>(item)->category() != AddresseeViewItem::Entry)
      {
        firstGroup = item;
      }
      ++found;
    }
    item = item->nextSibling();
  }

  if (found == 1 && firstGroup)
  {
    firstGroup->setOpen(true);
  }
}

void
AddressesDialog::availableSelectionChanged()
{
  bool selection = !selectedAvailableAddresses.isEmpty();
  d->ui->mToButton->setEnabled(selection);
  d->ui->mCCButton->setEnabled(selection);
  d->ui->mBCCButton->setEnabled(selection);
}

void
AddressesDialog::selectedSelectionChanged()
{
  bool selection = !selectedSelectedAddresses.isEmpty();
  d->ui->mRemoveButton->setEnabled(selection);
}

void
AddressesDialog::availableAddressSelected( AddresseeViewItem* item, bool selected )
{
  if (selected)
  {
    selectedAvailableAddresses.append(item);
  }
  else
  {
    selectedAvailableAddresses.remove(item);
  }
}

void
AddressesDialog::selectedAddressSelected( AddresseeViewItem* item, bool selected )
{
  // we have to avoid that a parent and a child is selected together
  // because in this case we get a double object deletion ( program crashes )
  // when removing the selected items from list
  AddresseeViewItem* parent = static_cast<AddresseeViewItem*>(((Q3ListViewItem*)item)->parent());
  if ( parent  && selected )
    parent->setSelected( false );
  if (selected)
  {
    selectedSelectedAddresses.append(item);
  }
  else
  {
    selectedSelectedAddresses.remove(item);
  }
  if ( selected ) {
    AddresseeViewItem* child = static_cast<AddresseeViewItem*>(item->firstChild());
    while (child) {
      child->setSelected( false );
      child = static_cast<AddresseeViewItem*>(child->nextSibling());
    }
  }
}

void
AddressesDialog::initConnections()
{
  connect( d->ui->mFilterEdit, SIGNAL(textChanged(const QString &)),
           SLOT(filterChanged(const QString &)) );
  connect( d->ui->mToButton, SIGNAL(clicked()),
           SLOT(addSelectedTo()) );
  connect( d->ui->mCCButton, SIGNAL(clicked()),
           SLOT(addSelectedCC()) );
  connect( d->ui->mBCCButton, SIGNAL(clicked()),
           SLOT(addSelectedBCC())  );
  connect( d->ui->mSaveAs, SIGNAL(clicked()),
           SLOT(saveAs())  );
  connect( d->ui->mRemoveButton, SIGNAL(clicked()),
           SLOT(removeEntry()) );
  connect( d->ui->mAvailableView, SIGNAL(selectionChanged()),
           SLOT(availableSelectionChanged())  );
  connect( d->ui->mAvailableView, SIGNAL(doubleClicked(Q3ListViewItem*)),
           SLOT(addSelectedTo()) );
  connect( d->ui->mSelectedView, SIGNAL(selectionChanged()),
           SLOT(selectedSelectionChanged()) );
  connect( d->ui->mSelectedView, SIGNAL(doubleClicked(Q3ListViewItem*)),
           SLOT(removeEntry()) );

#ifndef KDEPIM_NEW_DISTRLISTS
  connect( KABC::DistributionListWatcher::self(), SIGNAL( changed() ),
           this, SLOT( updateAvailableAddressees() ) );
#endif

  connect( KABC::StdAddressBook::self( true ), SIGNAL( addressBookChanged(AddressBook*) ),
           this, SLOT( updateAvailableAddressees() ) );
}

void
AddressesDialog::addAddresseeToAvailable( const KABC::Addressee& addr, AddresseeViewItem* defaultParent, bool useCategory )
{
  if ( addr.preferredEmail().isEmpty() )
    return;

  if ( useCategory ) {
    QStringList categories = addr.categories();

    for ( QStringList::Iterator it = categories.begin(); it != categories.end(); ++it ) {
      if ( !d->groupDict[ *it ] ) {  //we don't have the category yet
        AddresseeViewItem* category = new AddresseeViewItem( d->ui->mAvailableView, *it );
        d->groupDict.insert( *it,  category );
      }

      for ( int i = 0; i < addr.emails().count(); ++i ) {
        AddresseeViewItem* addressee = new AddresseeViewItem( d->groupDict[ *it ], addr, i );
        connect(addressee, SIGNAL(addressSelected(AddresseeViewItem*, bool)),
                this, SLOT(availableAddressSelected(AddresseeViewItem*, bool)));
      }
    }
  }

  bool noCategory = false;
  if ( useCategory ) {
    if ( addr.categories().isEmpty() )
      noCategory = true;
  } else
    noCategory = true;

  if ( defaultParent && noCategory ) { // only non-categorized items here
    AddresseeViewItem* addressee = new AddresseeViewItem( defaultParent, addr );
    connect(addressee, SIGNAL(addressSelected(AddresseeViewItem*, bool)),
            this, SLOT(availableAddressSelected(AddresseeViewItem*, bool)));
  }
}

void
AddressesDialog::addAddresseeToSelected( const KABC::Addressee& addr, AddresseeViewItem* defaultParent )
{
  if ( addr.preferredEmail().isEmpty() )
    return;

  if ( defaultParent ) {
    AddresseeViewItem *myChild = static_cast<AddresseeViewItem*>( defaultParent->firstChild() );
    while( myChild ) {
      if ( myChild->addressee().preferredEmail() == addr.preferredEmail() )
        return;//already got it
      myChild = static_cast<AddresseeViewItem*>( myChild->nextSibling() );
    }
    AddresseeViewItem* addressee = new AddresseeViewItem( defaultParent, addr );
    connect(addressee, SIGNAL(addressSelected(AddresseeViewItem*, bool)),
            this, SLOT(selectedAddressSelected(AddresseeViewItem*, bool)));
    defaultParent->setOpen( true );
  }

  d->ui->mSaveAs->setEnabled(true);
}

void
AddressesDialog::addAddresseesToSelected( AddresseeViewItem *parent,
                                          const Q3PtrList<AddresseeViewItem>& addresses )
{
  Q_ASSERT( parent );

  Q3PtrListIterator<AddresseeViewItem> itr( addresses );

  if (itr.current())
  {
    d->ui->mSaveAs->setEnabled(true);
  }

  while ( itr.current() ) {
    AddresseeViewItem* address = itr.current();
    ++itr;

    if (selectedToAvailableMapping.find(address) != 0)
    {
      continue;
    }

    AddresseeViewItem* newItem = 0;
    if (address->category() == AddresseeViewItem::Entry)
    {
      newItem = new AddresseeViewItem( parent, address->addressee() );
    }
    else if (address->category() == AddresseeViewItem::DistList)
    {
      newItem = new AddresseeViewItem( parent, address->name() );
    }
    else
    {
      newItem = new AddresseeViewItem( parent, address->name(), allAddressee( address ) );
    }

    address->setSelected( false );
    address->setVisible( false );
    selectedToAvailableMapping.insert(address, newItem);
    selectedToAvailableMapping.insert(newItem, address);
    connect(newItem, SIGNAL(addressSelected(AddresseeViewItem*, bool)),
            this, SLOT(selectedAddressSelected(AddresseeViewItem*, bool)));
  }

  parent->setOpen( true );
}

QStringList
AddressesDialog::entryToString( const KABC::Addressee::List& l ) const
{
  QStringList entries;

  for( KABC::Addressee::List::ConstIterator it = l.begin(); it != l.end(); ++it ) {
    entries += (*it).fullEmail();
  }
  return entries;
}

void
AddressesDialog::addSelectedTo()
{
  if ( !d->toItem )
  {
    d->toItem = new AddresseeViewItem( d->ui->mSelectedView, i18n("To"), AddresseeViewItem::To );
    connect(d->toItem, SIGNAL(addressSelected(AddresseeViewItem*, bool)),
            this, SLOT(selectedAddressSelected(AddresseeViewItem*, bool)));
  }

  addAddresseesToSelected( d->toItem, selectedAvailableAddresses );
  selectedAvailableAddresses.clear();

  if ( d->toItem->childCount() > 0 )
    d->toItem->setVisible( true );
  else
  {
    delete d->toItem;
    d->toItem = 0;
  }
}

void
AddressesDialog::addSelectedCC()
{
  if ( !d->ccItem )
  {
    d->ccItem = new AddresseeViewItem( d->ui->mSelectedView, i18n("CC"), AddresseeViewItem::CC );
    connect(d->ccItem , SIGNAL(addressSelected(AddresseeViewItem*, bool)),
            this, SLOT(selectedAddressSelected(AddresseeViewItem*, bool)));
  }

  addAddresseesToSelected( d->ccItem, selectedAvailableAddresses );
  selectedAvailableAddresses.clear();

  if ( d->ccItem->childCount() > 0 )
    d->ccItem->setVisible( true );
  else
  {
    delete d->ccItem;
    d->ccItem = 0;
  }
}

void
AddressesDialog::addSelectedBCC()
{
  if ( !d->bccItem )
  {
    d->bccItem = new AddresseeViewItem( d->ui->mSelectedView, i18n("BCC"), AddresseeViewItem::BCC );
    connect(d->bccItem , SIGNAL(addressSelected(AddresseeViewItem*, bool)),
            this, SLOT(selectedAddressSelected(AddresseeViewItem*, bool)));
  }

  addAddresseesToSelected( d->bccItem, selectedAvailableAddresses );
  selectedAvailableAddresses.clear();

  if ( d->bccItem->childCount() > 0 )
    d->bccItem->setVisible( true );
  else
  {
    delete d->bccItem;
    d->bccItem = 0;
  }
}

void AddressesDialog::unmapSelectedAddress(AddresseeViewItem* item)
{
  AddresseeViewItem* correspondingItem = selectedToAvailableMapping[item];
  if (correspondingItem)
  {
    correspondingItem->setVisible( true );
    selectedToAvailableMapping.remove( item );
    selectedToAvailableMapping.remove( correspondingItem );
  }

  AddresseeViewItem* child = static_cast<AddresseeViewItem*>(item->firstChild());
  while (child)
  {
    unmapSelectedAddress(child);
    child = static_cast<AddresseeViewItem*>(child->nextSibling());
  }
}

void
AddressesDialog::removeEntry()
{
  Q3PtrList<AddresseeViewItem> lst;
  bool resetTo  = false;
  bool resetCC  = false;
  bool resetBCC = false;

  lst.setAutoDelete( false );
  Q3PtrListIterator<AddresseeViewItem> it( selectedSelectedAddresses );
  while ( it.current() ) {
    AddresseeViewItem* item = it.current();
    ++it;
    if ( d->toItem == item )
      resetTo = true;
    else if ( d->ccItem == item )
      resetCC = true;
    else if( d->bccItem == item )
      resetBCC = true;
    // we may only append parent items
    unmapSelectedAddress(item);
    lst.append( item );
  }
  selectedSelectedAddresses.clear();
  lst.setAutoDelete( true );
  lst.clear();
  if ( resetTo )
    d->toItem  = 0;
  else if ( d->toItem && d->toItem->childCount() == 0 )
  {
    delete d->toItem;
    d->toItem = 0;
  }
  if ( resetCC )
    d->ccItem = 0;
  else if ( d->ccItem && d->ccItem->childCount() == 0 )
  {
    delete d->ccItem;
    d->ccItem = 0;
  }
  if ( resetBCC )
    d->bccItem  = 0;
  else if ( d->bccItem && d->bccItem->childCount() == 0 )
  {
    delete d->bccItem;
    d->bccItem = 0;
  }
  d->ui->mSaveAs->setEnabled(d->ui->mSelectedView->firstChild() != 0);
}

#ifdef KDEPIM_NEW_DISTRLISTS

// copied from kabcore.cpp :(
// KDE4: should be in libkabc I think
static KABC::Resource *requestResource( KABC::AddressBook* abook, QWidget *parent )
{
  const QList<KABC::Resource*> kabcResources = abook->resources();

  QList<KRES::Resource*> kresResources;
  QList<KABC::Resource*>::const_iterator resIt;
  for ( resIt = kabcResources.begin(); resIt != kabcResources.end(); ++resIt) {
    if ( (*resIt)->readOnly() ) {
      KRES::Resource *res = static_cast<KRES::Resource*>( *resIt );
      if ( res )
        kresResources.append( res );
    }
  }

  KRES::Resource *res = KRES::SelectDialog::getResource( kresResources, parent );
  return static_cast<KABC::Resource*>( res );
}
#endif

void
AddressesDialog::saveAs()
{
#ifndef KDEPIM_NEW_DISTRLISTS
  KABC::DistributionListManager manager( KABC::StdAddressBook::self( true ) );
  manager.load();
#endif

  if ( !d->ui->mSelectedView->firstChild() ) {
    KMessageBox::information( 0,
                              i18n("There are no addresses in your list. "
                                   "First add some addresses from your address book, "
                                   "then try again.") );
    return;
  }

  bool ok = false;
  QString name = KInputDialog::getText( i18n("New Distribution List"),
                                        i18n("Please enter name:"),
                                        QString(), &ok,
                                        this );
  if ( !ok || name.isEmpty() )
    return;

  bool alreadyExists = false;
#ifdef KDEPIM_NEW_DISTRLISTS
  KABC::AddressBook* abook = KABC::StdAddressBook::self( true );
  KPIM::DistributionList dlist = KPIM::DistributionList::findByName( abook, name );
  alreadyExists = !dlist.isEmpty();
#else
  alreadyExists = manager.list( name );
#endif

  if ( alreadyExists ) {
    KMessageBox::information( 0,
                              i18n( "<qt>Distribution list with the given name <b>%1</b> "
                                    "already exists. Please select a different name.</qt>" ,
                                name ) );
    return;
  }

#ifdef KDEPIM_NEW_DISTRLISTS
  KABC::Resource* resource = requestResource( abook, this );
  if ( !resource )
    return;

  dlist.setResource( resource );
  dlist.setName( name );
  KABC::Addressee::List addrl = allAddressee( d->ui->mSelectedView, false );
  for ( KABC::Addressee::List::iterator itr = addrl.begin();
        itr != addrl.end(); ++itr ) {
    dlist.insertEntry( *itr );
  }
  abook->insertAddressee( dlist );
#else
  KABC::DistributionList *dlist = new KABC::DistributionList( &manager, name );
  KABC::Addressee::List addrl = allAddressee( d->ui->mSelectedView, false );
  for ( KABC::Addressee::List::iterator itr = addrl.begin();
        itr != addrl.end(); ++itr ) {
    dlist->insertEntry( *itr );
  }

  manager.save();
#endif
}

void
AddressesDialog::launchAddressBook()
{
  KToolInvocation::startServiceByDesktopName( "kaddressbook", QString() );
}

void
AddressesDialog::filterChanged( const QString& txt )
{
  Q3ListViewItemIterator it( d->ui->mAvailableView );
  bool showAll = false;

  if ( txt.isEmpty() )
    showAll = true;

  while ( it.current() ) {
    AddresseeViewItem* item = static_cast<AddresseeViewItem*>( it.current() );
    ++it;
    if ( showAll ) {
      item->setVisible( true );
      if ( item->category() == AddresseeViewItem::Group )
        item->setOpen( false );//close to not have too many entries
      continue;
    }
    if ( item->category() == AddresseeViewItem::Entry ) {
      bool matches = item->matches( txt ) ;
      item->setVisible( matches );
      if ( matches && static_cast<Q3ListViewItem*>(item)->parent() ) {
          static_cast<Q3ListViewItem*>(item)->parent()->setOpen( true );//open the parents with found entries
      }
    }
  }
}

KABC::Addressee::List
AddressesDialog::allAddressee( K3ListView* view, bool onlySelected ) const
{
  KABC::Addressee::List lst;
  Q3ListViewItemIterator it( view );
  while ( it.current() ) {
    AddresseeViewItem* item = static_cast<AddresseeViewItem*>( it.current() );
    if ( !onlySelected || item->isSelected() ) {
      if ( item->category() != AddresseeViewItem::Entry  ) {
        AddresseeViewItem *myChild = static_cast<AddresseeViewItem*>( item->firstChild() );
        while( myChild ) {
          lst.append( myChild->addressee() );
          myChild = static_cast<AddresseeViewItem*>( myChild->nextSibling() );
        }
      } else {
        lst.append( item->addressee() );
      }
    }
    ++it;
  }

  return lst;
}

KABC::Addressee::List
AddressesDialog::allAddressee( AddresseeViewItem* parent ) const
{
  KABC::Addressee::List lst;

  if ( !parent ) return lst;

  if ( parent->category() == AddresseeViewItem::Entry )
  {
    lst.append( parent->addressee() );
    return lst;
  }

  AddresseeViewItem *myChild = static_cast<AddresseeViewItem*>( parent->firstChild() );
  while( myChild ) {
    if ( myChild->category() == AddresseeViewItem::FilledGroup )
      lst += myChild->addresses();
    else if ( !myChild->addressee().isEmpty() )
      lst.append( myChild->addressee() );
    myChild = static_cast<AddresseeViewItem*>( myChild->nextSibling() );
  }

  return lst;
}

QStringList
AddressesDialog::allDistributionLists( AddresseeViewItem* parent ) const
{
  QStringList lists;

  if ( !parent )
    return QStringList();

  AddresseeViewItem *item = static_cast<AddresseeViewItem*>( parent->firstChild() );
  while ( item ) {
    if ( item->category() == AddresseeViewItem::DistList && !item->name().isEmpty() )
      lists.append( item->name() );

    item = static_cast<AddresseeViewItem*>( item->nextSibling() );
  }

  return lists;
}

void
AddressesDialog::addDistributionLists()
{
  KABC::AddressBook* abook = KABC::StdAddressBook::self( true );

#ifdef KDEPIM_NEW_DISTRLISTS
  const QList<KPIM::DistributionList> distLists =
    KPIM::DistributionList::allDistributionLists( abook );
#else
  KABC::DistributionListManager manager( abook );
  manager.load();

  QStringList distLists = manager.listNames();
#endif

  if ( distLists.isEmpty() )
    return;

  AddresseeViewItem *topItem = new AddresseeViewItem( d->ui->mAvailableView,
                                                      i18n( "Distribution Lists" ) );

#ifdef KDEPIM_NEW_DISTRLISTS
  QList<KPIM::DistributionList>::ConstIterator listIt;
#else
  QStringList::Iterator listIt;
#endif
  for ( listIt = distLists.begin(); listIt != distLists.end(); ++listIt ) {
#ifdef KDEPIM_NEW_DISTRLISTS
    KPIM::DistributionList dlist = *listIt;
    KPIM::DistributionList::Entry::List entries = dlist.entries(abook);
#else
    KABC::DistributionList& dlist = *manager.list( *listIt );
    KABC::DistributionList::Entry::List entries = dlist.entries();
#endif

    AddresseeViewItem *item = new AddresseeViewItem( topItem, dlist.name() );
    connect( item, SIGNAL( addressSelected( AddresseeViewItem*, bool ) ),
             this, SLOT( availableAddressSelected( AddresseeViewItem*, bool ) ) );

#ifdef KDEPIM_NEW_DISTRLISTS
    KPIM::DistributionList::Entry::List::Iterator itemIt;
#else
    KABC::DistributionList::Entry::List::Iterator itemIt;
#endif
    for ( itemIt = entries.begin(); itemIt != entries.end(); ++itemIt )
      addAddresseeToAvailable( (*itemIt).addressee, item, false );
  }
}

} // namespace

#include "addressesdialog.moc"
