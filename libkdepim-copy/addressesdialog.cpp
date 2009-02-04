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
#include "ldapsearchdialog.h"
#include "distributionlist.h"

#include <kpimutils/email.h>

#include <kresources/selectdialog.h>

#include <kabc/resource.h>
#include <kabc/stdaddressbook.h>

#include <KDebug>
#include <KGlobal>
#include <KIconLoader>
#include <KInputDialog>
#include <KLineEdit>
#include <KLocale>
#include <KMessageBox>
#include <KPushButton>
#include <KRun>
#include <KStandardDirs>
#include <KToolInvocation>
#include <KVBox>

#include <QLayout>
#include <QWidget>
#include <QPixmap>
#include <QHeaderView>

namespace KPIM {

// private start :
struct AddresseeViewItem::AddresseeViewItemPrivate {
  KABC::Addressee               address;
  AddresseeViewItem::Category   category;
  KABC::Addressee::List         addresses;
};

struct AddressesDialog::AddressesDialogPrivate {
  AddressesDialogPrivate() :
    ui(0), personal(0), recent(0),
    toItem(0), ccItem(0), bccItem(0),
    ldapSearchDialog(0)
  {}

  AddressPickerUI *ui;

  AddresseeViewItem *personal;
  AddresseeViewItem *recent;

  AddresseeViewItem *toItem;
  AddresseeViewItem *ccItem;
  AddresseeViewItem *bccItem;

  QHash<QString, AddresseeViewItem*> groupDict;

  KABC::Addressee::List recentAddresses;
  LdapSearchDialog  *ldapSearchDialog;
};
// privates end

AddresseeViewItem::AddresseeViewItem( AddresseeViewItem *parent, const KABC::Addressee& addr,
                                      int emailIndex )
  : QObject( 0 ), QTreeWidgetItem( parent )
{
  d = new AddresseeViewItemPrivate;
  d->address = addr;
  d->category = Entry;

  setText( 0, addr.realName() );
  setText( 1, ( emailIndex == 0 ? addr.preferredEmail() : addr.emails()[ emailIndex ] ) );
  if ( text( 0 ).trimmed().isEmpty() )
    setText( 0, addr.preferredEmail() );

  if ( addr.photo().url().isEmpty() ) {
    if ( addr.photo().data().isNull() )
      setIcon( 0, KIconLoader::global()->loadIcon( "x-office-contact", KIconLoader::Small ) );
    else
      setIcon( 0, QPixmap::fromImage( addr.photo().data().scaled( 16, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation
) ) );
  } else {
    setIcon( 0, KIconLoader::global()->loadIcon( addr.photo().url(), KIconLoader::Small ) );
  }
}

AddresseeViewItem::AddresseeViewItem( QTreeWidget *lv, const QString& name, Category cat )
  : QObject(0), QTreeWidgetItem( lv )
{
  d = new AddresseeViewItemPrivate;
  d->category = cat;
  setText( 0, name );
}

AddresseeViewItem::AddresseeViewItem(  AddresseeViewItem *parent, const QString& name,
                                       const KABC::Addressee::List &lst )
  : QObject(0), QTreeWidgetItem( parent )
{
  d = new AddresseeViewItemPrivate;
  d->category = FilledGroup;
  d->addresses = lst;
  setText( 0, name );
  setText( 1, i18n("<group>") );
}

AddresseeViewItem::AddresseeViewItem(  AddresseeViewItem *parent, const QString &name )
  : QObject( 0 ), QTreeWidgetItem( parent )
{
  d = new AddresseeViewItemPrivate;
  d->category = DistList;

  setIcon( 0, KIconLoader::global()->loadIcon( "x-mail-distribution-list", KIconLoader::Small ) );
  setText( 0, name );
  setText( 1, i18n( "<group>" ) );
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

bool AddresseeViewItem::operator < ( const QTreeWidgetItem& other ) const
{
  if ( category() == Group || category() == Entry )
    return QTreeWidgetItem::operator < ( other );

  // We define our custom sort behavior so that the To, CC and BCC groups are always
  // sorted in that order, regardless of actual sort direction.

  const AddresseeViewItem &item = static_cast<const AddresseeViewItem&>( other );
  int a = static_cast<int>( category() );
  int b = static_cast<int>( item.category() );

  Qt::SortOrder sortOrder = treeWidget()->header()->sortIndicatorOrder();

  if ( sortOrder == Qt::AscendingOrder )
    return ( a < b );
  else
    return ( a > b );
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

  setInitialSize( QSize( 780, 450 ) );
  d->ui->mAvailableView->setFocus();
  d->ui->mAvailableView->header()->setResizeMode( QHeaderView::ResizeToContents );
  d->ui->mSelectedView->header()->setResizeMode( QHeaderView::ResizeToContents );
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
  }
  return d->toItem;
}

AddresseeViewItem* AddressesDialog::selectedCcItem()
{
  if ( !d->ccItem )
  {
    d->ccItem = new AddresseeViewItem( d->ui->mSelectedView, i18n("CC"), AddresseeViewItem::CC );
  }
  return d->ccItem;
}

AddresseeViewItem* AddressesDialog::selectedBccItem()
{
  if ( !d->bccItem )
  {
    d->bccItem = new AddresseeViewItem( d->ui->mSelectedView, i18n("BCC"), AddresseeViewItem::BCC );
  }
  return d->bccItem;
}

void
AddressesDialog::setSelectedTo( const QStringList& l )
{
  QString name, email;
  for ( QStringList::ConstIterator it = l.constBegin(); it != l.constEnd(); ++it ) {
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
  for ( QStringList::ConstIterator it = l.constBegin(); it != l.constEnd(); ++it ) {
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
  for ( QStringList::ConstIterator it = l.constBegin(); it != l.constEnd(); ++it ) {
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
    d->recent->setHidden( true );
    d->groupDict.insert( recentGroup, d->recent );
  }

  KABC::Addressee::List::ConstIterator it;
  for ( it = d->recentAddresses.constBegin(); it != d->recentAddresses.constEnd(); ++it )
    addAddresseeToAvailable( *it, d->recent );

  if ( d->recent->childCount() > 0 ) {
    d->recent->setHidden( false );
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
  for ( QStringList::ConstIterator it = dList.constBegin(); it != dList.constEnd(); ++it ) {
    const KABC::DistributionList *list = abook->findDistributionListByName( *it );
    if ( !list )
      continue;
    const KABC::DistributionList::Entry::List eList = list->entries();
    KABC::DistributionList::Entry::List::ConstIterator eit;
    for( eit = eList.constBegin(); eit != eList.constEnd(); ++eit ) {
      KABC::Addressee a = (*eit).addressee();
      if ( !a.preferredEmail().isEmpty() && !aList.contains( a ) ) {
        aList.append( a ) ;
      }
    }
  }
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
  d->personal = new AddresseeViewItem( d->ui->mAvailableView, personalGroup );
  d->personal->setHidden( true );
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
    d->personal->setHidden( false );
  }

  checkForSingleAvailableGroup();
}

void AddressesDialog::checkForSingleAvailableGroup()
{
  int itemIndex = 0;
  QTreeWidgetItem* item = d->ui->mAvailableView->topLevelItem( itemIndex );
  QTreeWidgetItem* firstGroup = 0;
  int found = 0;
  while ( item )
  {
    if ( !item->isHidden() )
    {
      if ( !firstGroup &&
           static_cast<AddresseeViewItem*>( item )->category() != AddresseeViewItem::Entry )
      {
        firstGroup = item;
      }
      ++found;
    }
    item = d->ui->mAvailableView->topLevelItem( ++itemIndex );
  }

  if ( found == 1 && firstGroup )
  {
    d->ui->mAvailableView->expandItem( firstGroup );
  }
}

void
AddressesDialog::availableSelectionChanged()
{
  bool selection = !d->ui->mAvailableView->selectedItems().isEmpty();
  d->ui->mToButton->setEnabled(selection);
  d->ui->mCCButton->setEnabled(selection);
  d->ui->mBCCButton->setEnabled(selection);
}

void
AddressesDialog::selectedSelectionChanged()
{
  bool selection = !d->ui->mSelectedView->selectedItems().isEmpty();
  d->ui->mRemoveButton->setEnabled(selection);
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
  connect( d->ui->mLdapSearch, SIGNAL(clicked()),
           SLOT(searchLdap())  );
  connect( d->ui->mRemoveButton, SIGNAL(clicked()),
           SLOT(removeEntry()) );
  connect( d->ui->mAvailableView, SIGNAL(itemSelectionChanged()),
           SLOT(availableSelectionChanged())  );
  connect( d->ui->mAvailableView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
           SLOT(addSelectedTo()) );
  connect( d->ui->mSelectedView, SIGNAL(itemSelectionChanged()),
           SLOT(selectedSelectionChanged()) );
  connect( d->ui->mSelectedView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
           SLOT(removeEntry()) );

  connect( KABC::StdAddressBook::self( true ), SIGNAL( addressBookChanged(AddressBook*) ),
           this, SLOT( updateAvailableAddressees() ) );
}

void
AddressesDialog::addAddresseeToAvailable( const KABC::Addressee& addr,
                                          AddresseeViewItem* defaultParent, bool useCategory )
{
  if ( addr.preferredEmail().isEmpty() )
    return;

  if ( useCategory ) {
    QStringList categories = addr.categories();

    for ( QStringList::Iterator it = categories.begin(); it != categories.end(); ++it ) {
      if ( !d->groupDict.contains( *it ) ) {  //we don't have the category yet
        AddresseeViewItem* category = new AddresseeViewItem( d->ui->mAvailableView, *it );
        d->groupDict.insert( *it,  category );
      }

      for ( int i = 0; i < addr.emails().count(); ++i ) {
        new AddresseeViewItem( d->groupDict.value( *it ), addr, i );
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
    new AddresseeViewItem( defaultParent, addr );
  }
}

void
AddressesDialog::addAddresseeToSelected( const KABC::Addressee& addr, AddresseeViewItem* defaultParent )
{
  if ( addr.preferredEmail().isEmpty() )
    return;

  if ( defaultParent ) {
    int childIndex = 0;
    AddresseeViewItem *myChild = static_cast<AddresseeViewItem*>(
        defaultParent->QTreeWidgetItem::child( childIndex ) );
    while( myChild ) {
      if ( myChild->addressee().preferredEmail() == addr.preferredEmail() )
        return;//already got it
      myChild = static_cast<AddresseeViewItem*>(
          defaultParent->QTreeWidgetItem::child( ++childIndex ) );
    }
    new AddresseeViewItem( defaultParent, addr );
    d->ui->mSelectedView->expandItem( defaultParent );
  }

  d->ui->mSaveAs->setEnabled(true);
}

void
AddressesDialog::addAddresseesToSelected( AddresseeViewItem *parent,
                                          const QList<AddresseeViewItem*> addresses )
{
  Q_ASSERT( parent );

  QList<AddresseeViewItem*>::const_iterator itr = addresses.begin();

  if ( itr != addresses.end() )
  {
    d->ui->mSaveAs->setEnabled(true);
  }

  while ( itr != addresses.end() ) {
    AddresseeViewItem* address = (*itr);
    ++itr;

    if ( selectedToAvailableMapping.find( address ) != selectedToAvailableMapping.end() )
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
    address->setHidden( true );
    selectedToAvailableMapping.insert(address, newItem);
    selectedToAvailableMapping.insert(newItem, address);
  }

  d->ui->mSelectedView->expandItem( parent );
}

QStringList
AddressesDialog::entryToString( const KABC::Addressee::List& l ) const
{
  QStringList entries;

  for( KABC::Addressee::List::ConstIterator it = l.constBegin(); it != l.constEnd(); ++it ) {
    entries += (*it).fullEmail();
  }
  return entries;
}

QList<AddresseeViewItem*> AddressesDialog::selectedAvailableAddresses() const
{
  QList<AddresseeViewItem*> list;
  foreach( QTreeWidgetItem* item, d->ui->mAvailableView->selectedItems() ) {
    list.append( static_cast<AddresseeViewItem*>( item ) );
  }
  return list;
}

QList<AddresseeViewItem*> AddressesDialog::selectedSelectedAddresses() const
{
  QList<AddresseeViewItem*> list;;
  foreach( QTreeWidgetItem* item, d->ui->mSelectedView->selectedItems() ) {
    list.append( static_cast<AddresseeViewItem*>( item ) );
  }
  return list;
}

void
AddressesDialog::addSelectedTo()
{
  if ( !d->toItem )
  {
    d->toItem = new AddresseeViewItem( d->ui->mSelectedView, i18n("To"), AddresseeViewItem::To );
  }

  addAddresseesToSelected( d->toItem, selectedAvailableAddresses() );

  if ( d->toItem->childCount() > 0 )
    d->toItem->setHidden( false );
  else
  {
    delete d->toItem;
    d->toItem = 0;
  }
  availableSelectionChanged();
}

void
AddressesDialog::addSelectedCC()
{
  if ( !d->ccItem )
  {
    d->ccItem = new AddresseeViewItem( d->ui->mSelectedView, i18n("CC"), AddresseeViewItem::CC );
  }

  addAddresseesToSelected( d->ccItem, selectedAvailableAddresses() );

  if ( d->ccItem->childCount() > 0 )
    d->ccItem->setHidden( false );
  else
  {
    delete d->ccItem;
    d->ccItem = 0;
  }
  availableSelectionChanged();
}

void
AddressesDialog::addSelectedBCC()
{
  if ( !d->bccItem )
  {
    d->bccItem = new AddresseeViewItem( d->ui->mSelectedView, i18n("BCC"), AddresseeViewItem::BCC );
  }

  addAddresseesToSelected( d->bccItem, selectedAvailableAddresses() );

  if ( d->bccItem->childCount() > 0 )
    d->bccItem->setHidden( false );
  else
  {
    delete d->bccItem;
    d->bccItem = 0;
  }
  availableSelectionChanged();
}

void AddressesDialog::unmapSelectedAddress(AddresseeViewItem* item)
{
  AddresseeViewItem* correspondingItem = selectedToAvailableMapping[item];
  if (correspondingItem)
  {
    correspondingItem->setHidden( false );
    selectedToAvailableMapping.remove( item );
    selectedToAvailableMapping.remove( correspondingItem );
  }

  int childIndex = 0;
  AddresseeViewItem* child = static_cast<AddresseeViewItem*>(
      item->QTreeWidgetItem::child( childIndex) );
  while ( child )
  {
    unmapSelectedAddress( child );
    child = static_cast<AddresseeViewItem*>(
        item->QTreeWidgetItem::child( ++childIndex ) );
  }
}

void
AddressesDialog::removeEntry()
{
  QList<AddresseeViewItem*> lst;
  bool resetTo  = false;
  bool resetCC  = false;
  bool resetBCC = false;

  QList<AddresseeViewItem*> selectedItems = selectedSelectedAddresses();
  QList<AddresseeViewItem*>::iterator it = selectedItems.begin();
  while ( it != selectedItems.end() ) {
    AddresseeViewItem* item = (*it);
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
  qDeleteAll( lst );
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
  d->ui->mSaveAs->setEnabled( d->ui->mSelectedView->topLevelItemCount() > 0 );
  selectedSelectionChanged();
}

// copied from kabcore.cpp :(
// KDE4: should be in libkabc I think
static KABC::Resource *requestResource( KABC::AddressBook* abook, QWidget *parent )
{
  const QList<KABC::Resource*> kabcResources = abook->resources();

  QList<KRES::Resource*> kresResources;
  QList<KABC::Resource*>::const_iterator resIt;
  for ( resIt = kabcResources.begin(); resIt != kabcResources.end(); ++resIt) {
    if ( !(*resIt)->readOnly() ) {
      KRES::Resource *res = static_cast<KRES::Resource*>( *resIt );
      if ( res )
        kresResources.append( res );
    }
  }

  if ( kresResources.size() > 0 ) {
    KRES::Resource *res = KRES::SelectDialog::getResource( kresResources, parent );
    return static_cast<KABC::Resource*>( res );
  }
  else return 0;
}

void
AddressesDialog::saveAs()
{
  if ( d->ui->mSelectedView->topLevelItemCount() == 0 ) {
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
  KABC::AddressBook* abook = KABC::StdAddressBook::self( true );
  KABC::DistributionList *dlist = abook->findDistributionListByName( name );
  alreadyExists = (dlist != 0);

  if ( alreadyExists ) {
    KMessageBox::information( 0,
                              i18n( "<qt>Distribution list with the given name <b>%1</b> "
                                    "already exists. Please select a different name.</qt>" ,
                                name ) );
    return;
  }

  KABC::Resource* resource = requestResource( abook, this );
  if ( !resource )
    return;

  dlist = new KABC::DistributionList( resource, name );
  KABC::Addressee::List addrl = allAddressee( d->ui->mSelectedView, false );
  for ( KABC::Addressee::List::iterator itr = addrl.begin();
        itr != addrl.end(); ++itr ) {
    dlist->insertEntry( *itr );
  }
}

void
AddressesDialog::searchLdap()
{
    if ( !d->ldapSearchDialog ) {
      d->ldapSearchDialog = new LdapSearchDialog( this );
      connect( d->ldapSearchDialog, SIGNAL( addresseesAdded() ),
               SLOT(ldapSearchResult() ) );
    }
    d->ldapSearchDialog->show();
}

void
AddressesDialog::ldapSearchResult()
{
  QStringList emails = d->ldapSearchDialog->selectedEMails().split(',');
  QStringList::iterator it( emails.begin() );
  QStringList::iterator end( emails.end() );
  for ( ; it != end; ++it ){
      QString name;
      QString email;
      KPIMUtils::extractEmailAddressAndName( (*it), email, name );
      KABC::Addressee ad;
      ad.setNameFromString( name );
      ad.insertEmail( email );
      addAddresseeToSelected( ad, selectedToItem() );
  }
}

void
AddressesDialog::filterChanged( const QString& txt )
{
  QTreeWidgetItemIterator it( d->ui->mAvailableView );
  bool showAll = false;

  if ( txt.isEmpty() )
    showAll = true;

  while ( (*it) ) {
    AddresseeViewItem* item = static_cast<AddresseeViewItem*>( (*it) );
    ++it;
    if ( showAll ) {
      item->setHidden( false );
      if ( item->category() == AddresseeViewItem::Group )
        d->ui->mAvailableView->collapseItem( item );//close to not have too many entries
      continue;
    }
    if ( item->category() == AddresseeViewItem::Entry ) {
      bool matches = item->matches( txt ) ;
      item->setHidden( !matches );
      QTreeWidgetItem *parent = item->QTreeWidgetItem::parent();
      if ( matches && parent ) {
        d->ui->mAvailableView->expandItem( parent );//open the parents with found entries
      }
    }
  }
}

KABC::Addressee::List
AddressesDialog::allAddressee( QTreeWidget* view, bool onlySelected ) const
{
  KABC::Addressee::List lst;
  QTreeWidgetItemIterator it( view );
  while ( *it ) {
    AddresseeViewItem* item = static_cast<AddresseeViewItem*>( *it );
    if ( !onlySelected || item->isSelected() ) {
      if ( item->category() != AddresseeViewItem::Entry  ) {
        int childIndex = 0;
        AddresseeViewItem *myChild = static_cast<AddresseeViewItem*>(
            item->QTreeWidgetItem::child( childIndex ) );
        while( myChild ) {
          lst.append( myChild->addressee() );
          myChild = static_cast<AddresseeViewItem*>(
              item->QTreeWidgetItem::child( ++childIndex ) );
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

  int childIndex = 0;
  AddresseeViewItem *myChild = static_cast<AddresseeViewItem*>(
      parent->QTreeWidgetItem::child( childIndex ) );
  while( myChild ) {
    if ( myChild->category() == AddresseeViewItem::FilledGroup )
      lst += myChild->addresses();
    else if ( !myChild->addressee().isEmpty() )
      lst.append( myChild->addressee() );
    myChild = static_cast<AddresseeViewItem*>(
        parent->QTreeWidgetItem::child( ++childIndex ) );
  }

  return lst;
}

QStringList
AddressesDialog::allDistributionLists( AddresseeViewItem* parent ) const
{
  QStringList lists;

  if ( !parent )
    return QStringList();

  int childIndex = 0;
  AddresseeViewItem *item = static_cast<AddresseeViewItem*>(
      parent->QTreeWidgetItem::child( childIndex ) );
  while ( item ) {
    if ( item->category() == AddresseeViewItem::DistList && !item->name().isEmpty() )
      lists.append( item->name() );

    item = static_cast<AddresseeViewItem*>(
        parent->QTreeWidgetItem::child( ++childIndex ) );
  }

  return lists;
}

void
AddressesDialog::addDistributionLists()
{
  KABC::AddressBook* abook = KABC::StdAddressBook::self( true );

  const QList<KABC::DistributionList*> distLists = abook->allDistributionLists();
  if ( distLists.isEmpty() )
    return;

  AddresseeViewItem *topItem = new AddresseeViewItem( d->ui->mAvailableView,
                                                      i18n( "Distribution Lists" ) );

  QList<KABC::DistributionList*>::ConstIterator listIt;
  for ( listIt = distLists.constBegin(); listIt != distLists.constEnd(); ++listIt ) {
    KABC::DistributionList *dlist = *listIt;
    KABC::DistributionList::Entry::List entries = dlist->entries();

    AddresseeViewItem *item = new AddresseeViewItem( topItem, dlist->name() );

    KABC::DistributionList::Entry::List::ConstIterator itemIt;
    for ( itemIt = entries.constBegin(); itemIt != entries.constEnd(); ++itemIt )
      addAddresseeToAvailable( (*itemIt).addressee(), item, false );
  }
}

} // namespace

#include "addressesdialog.moc"
