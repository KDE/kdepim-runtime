/*  -*- mode: C++; c-file-style: "gnu" -*-
 *
 *  This file is part of libkdepim.
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
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 *
 */
#include <kabc/stdaddressbook.h>
#include <kabc/distributionlist.h>

#include <kmessagebox.h>
#include <kpushbutton.h>
#include <klineedit.h>
#include <krun.h>
#include <kstandarddirs.h>
#include <kprocess.h>
#include <klocale.h>
#include <kdebug.h>
#include <kinputdialog.h>
#include <qlayout.h>
#include <qvbox.h>
#include <qwidget.h>
#include <qdict.h>

#include "addressesdialog.h"
#include "addresspicker.h"

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

  QDict<AddresseeViewItem>     groupDict;
};
// privates end

AddresseeViewItem::AddresseeViewItem( AddresseeViewItem *parent, const KABC::Addressee& addr )
  : QObject(0), KListViewItem( parent, addr.realName(), addr.preferredEmail() )
{
  d = new AddresseeViewItemPrivate;
  d->address = addr;
  d->category = Entry;

  if ( text(0).stripWhiteSpace().isEmpty() )
    setText( 0, addr.preferredEmail() );
}

AddresseeViewItem::AddresseeViewItem( KListView *lv, const QString& name, Category cat )
  : QObject(0), KListViewItem( lv, name )
{
  d = new AddresseeViewItemPrivate;
  d->category = cat;
}

AddresseeViewItem::AddresseeViewItem(  AddresseeViewItem *parent, const QString& name,
                                       const KABC::Addressee::List &lst )
  : QObject(0), KListViewItem( parent, name, i18n("<group>") )
{
  d = new AddresseeViewItemPrivate;
  d->category = FilledGroup;
  d->addresses = lst;
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
    return d->address.realName().contains(txt) || d->address.preferredEmail().contains(txt);
}

void AddresseeViewItem::setSelected(bool selected)
{
    if (selected == isSelected())
    {
        return;
    }
    emit addressSelected( this, selected );
    QListViewItem::setSelected(selected);
}

int
AddresseeViewItem::compare( QListViewItem * i, int col, bool ascending ) const
{
  if ( category() == Group || category() == Entry )
    return KListViewItem::compare( i , col, ascending );

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

AddressesDialog::AddressesDialog( QWidget *widget, const char *name )
  : KDialogBase( widget, name, true, i18n("Address Selection"),
                 Ok|Cancel, Ok, true )
{
  QVBox *page = makeVBoxMainWidget();
  d = new AddressesDialogPrivate;
  d->ui = new AddressPickerUI( page );

  updateAvailableAddressees();
  initConnections();
}

AddressesDialog::~AddressesDialog()
{
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
AddressesDialog::setRecentAddresses( const KABC::Addressee::List& addr )
{
  static const QString &recentGroup = KGlobal::staticQString( i18n( "Recent Addresses" ) );
  if ( !d->recent ) {
    d->recent = new AddresseeViewItem( d->ui->mAvailableView, recentGroup );
    connect(d->recent, SIGNAL(addressSelected(AddresseeViewItem*, bool)),
            this, SLOT(availableAddressSelected(AddresseeViewItem*, bool)));
    d->recent->setVisible( false );
    d->groupDict.insert( recentGroup, d->recent );
  }

  for( KABC::Addressee::List::ConstIterator it = addr.begin();
       it != addr.end(); ++it ) {
    addAddresseeToAvailable( *it, d->recent );
  }
  if ( d->recent->childCount() > 0 ) {
    d->recent->setVisible( true );
  }

  checkForSingleAvailableGroup();
}

void
AddressesDialog::setShowCC( bool b )
{
  d->ui->mCCButton->setShown( b );
}

void
AddressesDialog::setShowBCC( bool b )
{
  d->ui->mBCCButton->setShown( b );
}

QStringList
AddressesDialog::to() const
{
  KABC::Addressee::List l = toAddresses();
  return entryToString( l );
}

QStringList
AddressesDialog::cc() const
{
  KABC::Addressee::List l = ccAddresses();
  return entryToString( l );
}

QStringList
AddressesDialog::bcc() const
{
  KABC::Addressee::List l = bccAddresses();
  return entryToString( l );
}

KABC::Addressee::List
AddressesDialog::toAddresses()  const
{
  return allAddressee( d->toItem );
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

void
AddressesDialog::updateAvailableAddressees()
{
  d->ui->mAvailableView->clear();

  static const QString &personalGroup = KGlobal::staticQString( i18n( "Other Addresses" ) );
  d->ui->mAvailableView->setRootIsDecorated( true );
  d->personal = new AddresseeViewItem( d->ui->mAvailableView, personalGroup );
  connect(d->personal, SIGNAL(addressSelected(AddresseeViewItem*, bool)),
            this, SLOT(selectedAddressSelected(AddresseeViewItem*, bool)));
  d->personal->setVisible( false );
  d->groupDict.insert( personalGroup, d->personal );

  KABC::AddressBook *addressBook = KABC::StdAddressBook::self();
  for( KABC::AddressBook::Iterator it = addressBook->begin();
       it != addressBook->end(); ++it ) {
    addAddresseeToAvailable( *it, d->personal );
  }

  addDistributionLists();
  if ( d->personal->childCount() > 0 ) {
    d->personal->setVisible( true );
  }

  checkForSingleAvailableGroup();
}

void AddressesDialog::checkForSingleAvailableGroup()
{
  QListViewItem* item = d->ui->mAvailableView->firstChild();
  QListViewItem* firstGroup = 0;
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
  d->ui->mEditButton->setEnabled(selection);
  d->ui->mDeleteButton->setEnabled(selection);
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
  if (selected)
  {
    selectedSelectedAddresses.append(item);
  }
  else
  {
    selectedSelectedAddresses.remove(item);
  }
}

void
AddressesDialog::initConnections()
{
  QObject::connect( d->ui->mDeleteButton, SIGNAL(clicked()),
                    SLOT(deleteEntry()) );
  QObject::connect( d->ui->mNewButton, SIGNAL(clicked()),
                    SLOT(newEntry()) );
  QObject::connect( d->ui->mEditButton, SIGNAL(clicked()),
                    SLOT(editEntry()) );
  QObject::connect( d->ui->mFilterEdit, SIGNAL(textChanged(const QString &)),
                    SLOT(filterChanged(const QString &)) );
  QObject::connect( d->ui->mToButton, SIGNAL(clicked()),
                    SLOT(addSelectedTo()) );
  QObject::connect( d->ui->mCCButton, SIGNAL(clicked()),
                    SLOT(addSelectedCC()) );
  QObject::connect( d->ui->mBCCButton, SIGNAL(clicked()),
                    SLOT(addSelectedBCC())  );
  QObject::connect( d->ui->mSaveAs, SIGNAL(clicked()),
                    SLOT(saveAs())  );
  QObject::connect( d->ui->mRemoveButton, SIGNAL(clicked()),
                    SLOT(removeEntry()) );
  QObject::connect( d->ui->mAvailableView, SIGNAL(selectionChanged()),
                    SLOT(availableSelectionChanged())  );
  QObject::connect( d->ui->mAvailableView, SIGNAL(doubleClicked(QListViewItem*)),
                    SLOT(addSelectedTo()) );
  QObject::connect( d->ui->mSelectedView, SIGNAL(selectionChanged()),
                    SLOT(selectedSelectionChanged()) );
  QObject::connect( d->ui->mSelectedView, SIGNAL(doubleClicked(QListViewItem*)),
                    SLOT(removeEntry()) );

  connect( KABC::DistributionListWatcher::self(), SIGNAL( changed() ),
           this, SLOT( updateAvailableAddressees() ) );

  connect( KABC::StdAddressBook::self(), SIGNAL( addressBookChanged(AddressBook*) ),
           this, SLOT( updateAvailableAddressees() ) );
}

void
AddressesDialog::addAddresseeToAvailable( const KABC::Addressee& addr, AddresseeViewItem* defaultParent )
{
  if ( addr.preferredEmail().isEmpty() )
    return;

  QStringList categories = addr.categories();

  for ( QStringList::Iterator it = categories.begin(); it != categories.end(); ++it ) {
    AddresseeViewItem* addressee = 0;
    if ( d->groupDict[ *it ] ) {
      addressee = new AddresseeViewItem( d->groupDict[ *it ], addr );
    } else {
      addressee = new AddresseeViewItem( d->ui->mAvailableView, *it );
      d->groupDict.insert( *it,  addressee);
    }
    connect(addressee, SIGNAL(addressSelected(AddresseeViewItem*, bool)),
            this, SLOT(availableAddressSelected(AddresseeViewItem*, bool)));
  }

  if ( defaultParent && categories.isEmpty() ) { // only non-categorized items here
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
                                          const QPtrList<AddresseeViewItem>& addresses )
{
  Q_ASSERT( parent );

  QPtrListIterator<AddresseeViewItem> itr( addresses );

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
  QPtrList<AddresseeViewItem> lst;
  bool resetTo  = false;
  bool resetCC  = false;
  bool resetBCC = false;

  lst.setAutoDelete( false );

  QPtrListIterator<AddresseeViewItem> it( selectedSelectedAddresses );
  while ( it.current() ) {
    AddresseeViewItem* item = it.current();
    ++it;
    if ( d->toItem == item )
      resetTo = true;
    else if ( d->ccItem == item )
      resetCC = true;
    else if( d->bccItem == item )
      resetBCC = true;

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

void
AddressesDialog::saveAs()
{
  KABC::DistributionListManager manager( KABC::StdAddressBook::self() );
  manager.load();

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
                                        QString::null, &ok,
                                        this );
  if ( !ok || name.isEmpty() )
    return;

  if ( manager.list( name ) ) {
    KMessageBox::information( 0,
                              i18n( "<qt>Distribution list with the given name <b>%1</b> "
                                    "already exists. Please select a different name.</qt>" )
                              .arg( name ) );
  }

  KABC::DistributionList *dlist = new KABC::DistributionList( &manager, name );
  KABC::Addressee::List addrl = allAddressee( d->ui->mSelectedView, false );
  for ( KABC::Addressee::List::iterator itr = addrl.begin();
        itr != addrl.end(); ++itr ) {
    dlist->insertEntry( *itr );
  }

  manager.save();
}

void
AddressesDialog::editEntry()
{
  AddresseeViewItem *item = static_cast<AddresseeViewItem*>( d->ui->mAvailableView->currentItem() );

#if defined( Q_CC_GNU )
#warning "This is rather crappy"
#endif
  if ( item ) {
    if ( ! KStandardDirs::findExe( "kaddressbook" ).isEmpty()  ) {
      KRun::runCommand( "kaddressbook -a " + KProcess::quote( item->addressee().fullEmail() ) );
    } else {
      KMessageBox::information( 0,
                                i18n("No external address book application found. You might want to "
                                     "install KAddressBook from the kdepim module.") );
    }
  } else {
    KMessageBox::information( 0,
                              i18n("Please select the entry which you want to edit.") );
  }
}

void
AddressesDialog::newEntry()
{
#if defined( Q_CC_GNU )
#warning "FIXME: do not call kaddressbook"
#endif
  KRun::runCommand( "kaddressbook --editor-only --new-contact" );
}

void
AddressesDialog::deleteEntry()
{
}

void
AddressesDialog::filterChanged( const QString& txt )
{
  QListViewItemIterator it( d->ui->mAvailableView );
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
      if ( matches && static_cast<QListViewItem*>(item)->parent() ) {
          static_cast<QListViewItem*>(item)->parent()->setOpen( true );//open the parents with found entries
      }
    }
  }
}

KABC::Addressee::List
AddressesDialog::allAddressee( KListView* view, bool onlySelected ) const
{
  KABC::Addressee::List lst;
  QListViewItemIterator it( view );
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

  if ( !parent ) return KABC::Addressee::List();

  if ( parent->category() == AddresseeViewItem::Entry )
  {
      lst.append( parent->addressee() );
      return lst;
  }

  AddresseeViewItem *myChild = static_cast<AddresseeViewItem*>( parent->firstChild() );
  while( myChild ) {
    if ( myChild->category() == AddresseeViewItem::FilledGroup )
      lst += myChild->addresses();
    else
      lst.append( myChild->addressee() );
    myChild = static_cast<AddresseeViewItem*>( myChild->nextSibling() );
  }

  return lst;
}

void
AddressesDialog::addDistributionLists()
{
  KABC::DistributionListManager *manager =
    new KABC::DistributionListManager( KABC::StdAddressBook::self() );
  manager->load();

  QStringList distLists = manager->listNames();

  for( QStringList::iterator itr = distLists.begin(); itr != distLists.end(); ++itr ) {
    KABC::DistributionList* dlist = manager->list( *itr );
    KABC::DistributionList::Entry::List elist = dlist->entries();
    AddresseeViewItem *parent = new AddresseeViewItem( d->ui->mAvailableView, dlist->name() );
    connect(parent, SIGNAL(addressSelected(AddresseeViewItem*, bool)),
            this, SLOT(selectedAddressSelected(AddresseeViewItem*, bool)));

    for( KABC::DistributionList::Entry::List::iterator itr = elist.begin();
         itr != elist.end(); ++itr ) {
      addAddresseeToAvailable( (*itr).addressee, parent );
    }
  }
}

}//end namespace KPIM

#include "addressesdialog.moc"
