/*  -*- mode: C++; c-file-style: "gnu" -*-
 *
 *  This file is part of libkdepim.
 *  Copyright (c) 2003 Zack Rusin <zack@kde.org>
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
#include <qinputdialog.h>
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
  : KListViewItem( parent, addr.realName(), addr.preferredEmail() )
{
  d = new AddresseeViewItemPrivate;
  d->address = addr;
  d->category = Entry;
}

AddresseeViewItem::AddresseeViewItem( KListView *lv, const QString& name, Category cat )
  : KListViewItem( lv, name )
{
  d = new AddresseeViewItemPrivate;
  d->category = cat;
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

QString AddressesDialog::sPersonalGroup = i18n("Personal Address Book");
QString AddressesDialog::sRecentGroup = i18n("Recent Addresses");

AddressesDialog::AddressesDialog( QWidget *widget, const char *name )
  : KDialogBase( widget, name, true, i18n("Address Selection"),
                 Ok|Cancel, Ok, true )
{
  QVBox *page = makeVBoxMainWidget();
  d = new AddressesDialogPrivate;
  d->ui = new AddressPickerUI( page );

  initGUI();
  initConnections();
}

AddressesDialog::~AddressesDialog()
{
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
    if ( !d->toItem )
      d->toItem = new AddresseeViewItem( d->ui->mSelectedView, i18n("To"), AddresseeViewItem::To );
    addAddresseeToSelected( addr, d->toItem );
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
    if ( !d->ccItem )
      d->ccItem = new AddresseeViewItem( d->ui->mSelectedView, i18n("CC"), AddresseeViewItem::CC );
    addAddresseeToSelected( addr, d->ccItem );
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
    if ( !d->bccItem )
      d->bccItem = new AddresseeViewItem( d->ui->mSelectedView, i18n("BCC"), AddresseeViewItem::BCC );
    addAddresseeToSelected( addr, d->bccItem );
  }
}

void
AddressesDialog::setRecentAddresses( const KABC::Addressee::List& addr )
{
  if ( !d->recent ) {
    d->recent   = new AddresseeViewItem( d->ui->mAvailableView, sRecentGroup );
    d->recent->setVisible( false );
    d->groupDict.insert( sRecentGroup, d->recent );
  }

  for( KABC::Addressee::List::ConstIterator it = addr.begin();
       it != addr.end(); ++it ) {
    addAddresseeToAvailable( *it, d->recent );
  }
  if ( d->recent->childCount() > 0 ) {
    d->recent->setVisible( true );
  }
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
AddressesDialog::initGUI()
{
  d->ui->mAvailableView->setRootIsDecorated( true );
  d->personal = new AddresseeViewItem( d->ui->mAvailableView, sPersonalGroup );
  d->personal->setVisible( false );
  d->groupDict.insert( sPersonalGroup, d->personal );

  KABC::AddressBook *addressBook = KABC::StdAddressBook::self();
  for( KABC::AddressBook::Iterator it = addressBook->begin();
       it != addressBook->end(); ++it ) {
    addAddresseeToAvailable( *it, d->personal );
  }

  addDistributionLists();
  if ( d->personal->childCount() > 0 ) {
    d->personal->setVisible( true );
  }
}

void
AddressesDialog::initConnections()
{
  QObject::connect( d->ui->mDeleteButton, SIGNAL(pressed()),
                    SLOT(deleteEntry()) );
  QObject::connect( d->ui->mNewButton, SIGNAL(pressed()),
                    SLOT(newEntry())  );
  QObject::connect( d->ui->mEditButton, SIGNAL(pressed()),
                    SLOT(editEntry())  );
  QObject::connect( d->ui->mClearButton, SIGNAL(pressed()),
                    SLOT(cleanEdit())  );
  QObject::connect( d->ui->mFilterEdit, SIGNAL(textChanged(const QString &)),
                    SLOT(filterChanged(const QString &)) );
  QObject::connect( d->ui->mToButton, SIGNAL(pressed()),
                    SLOT(addSelectedTo())  );
  QObject::connect( d->ui->mCCButton, SIGNAL(pressed()),
                    SLOT(addSelectedCC())  );
  QObject::connect( d->ui->mBCCButton, SIGNAL(pressed()),
                    SLOT(addSelectedBCC())  );
  QObject::connect( d->ui->mSaveAs, SIGNAL(pressed()),
                    SLOT(saveAs())  );
  QObject::connect( d->ui->mRemoveButton, SIGNAL(pressed()),
                    SLOT(removeEntry())  );
}

void
AddressesDialog::addAddresseeToAvailable( const KABC::Addressee& addr, AddresseeViewItem* defaultParent )
{
  if ( addr.preferredEmail().isEmpty() )
    return;

  QStringList categories = addr.categories();

  for ( QStringList::Iterator it = categories.begin(); it != categories.end(); ++it ) {
    if ( d->groupDict[ *it ] ) {
      new AddresseeViewItem( d->groupDict[ *it ], addr );
    } else {
      d->groupDict.insert( *it, new AddresseeViewItem( d->ui->mAvailableView, *it ) );
    }
  }

  if ( defaultParent ) {
    new AddresseeViewItem( defaultParent, addr );
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
    new AddresseeViewItem( defaultParent, addr );
  }
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
  KABC::Addressee::List lst = selectedAddressee( d->ui->mAvailableView );
  if ( !d->toItem )
    d->toItem = new AddresseeViewItem( d->ui->mSelectedView, i18n("To"), AddresseeViewItem::To );

  for( KABC::Addressee::List::Iterator it = lst.begin(); it != lst.end(); ++it ) {
    addAddresseeToSelected( *it, d->toItem );
  }
  if ( d->toItem->childCount() > 0 )
    d->toItem->setVisible( true );
}

void
AddressesDialog::addSelectedCC()
{
  KABC::Addressee::List lst = selectedAddressee( d->ui->mAvailableView );
  if ( !d->ccItem )
    d->ccItem = new AddresseeViewItem( d->ui->mSelectedView, i18n("CC"), AddresseeViewItem::CC );

  for( KABC::Addressee::List::Iterator it = lst.begin(); it != lst.end(); ++it ) {
    addAddresseeToSelected( *it, d->ccItem );
  }
  if ( d->ccItem->childCount() > 0 )
    d->ccItem->setVisible( true );
}

void
AddressesDialog::addSelectedBCC()
{
  KABC::Addressee::List lst = selectedAddressee( d->ui->mAvailableView );
  if ( !d->bccItem )
    d->bccItem = new AddresseeViewItem( d->ui->mSelectedView, i18n("BCC"), AddresseeViewItem::BCC );

  for( KABC::Addressee::List::Iterator it = lst.begin(); it != lst.end(); ++it ) {
    addAddresseeToSelected( *it, d->bccItem );
  }
  if ( d->bccItem->childCount() > 0 )
    d->bccItem->setVisible( true );
}

void
AddressesDialog::removeEntry()
{
  QPtrList<AddresseeViewItem> lst;
  bool resetTo  = false;
  bool resetCC  = false;
  bool resetBCC = false;

  lst.setAutoDelete( false );

  QListViewItemIterator it( d->ui->mSelectedView );
  while ( it.current() ) {
    AddresseeViewItem* item = static_cast<AddresseeViewItem*>( it.current() );
    ++it;
    if ( item->isSelected() ) {
      if ( d->toItem == item )
        resetTo = true;
      else if ( d->ccItem == item )
        resetCC = true;
      else if( d->bccItem == item )
        resetBCC = true;
      lst.append( item );
    }
  }

  lst.setAutoDelete( true );
  lst.clear();
  if ( resetTo )
    d->toItem  = 0;
  else if ( d->toItem && d->toItem->childCount() == 0 )
    d->toItem->setVisible( false );

  if ( resetCC )
    d->ccItem = 0;
  else if ( d->ccItem && d->ccItem->childCount() == 0 )
    d->ccItem->setVisible( false );

  if ( resetBCC )
    d->bccItem  = 0;
  else if ( d->bccItem && d->bccItem->childCount() == 0 )
    d->bccItem->setVisible( false );
}

void
AddressesDialog::saveAs()
{
  KABC::DistributionListManager manager( KABC::StdAddressBook::self() );
  manager.load();

  KABC::Addressee::List addrl = selectedAddressee( d->ui->mSelectedView );
  if ( addrl.count() == 0 ) {
    KMessageBox::information( 0,
                              i18n("You have to select the addressees which you wish to save "
                                   "as distribution list members.") );
    return;
  }

  bool ok = false;
  QString name = QInputDialog::getText( i18n("New Distribution List"),
                                        i18n("Please enter name."),
                                        QLineEdit::Normal, QString::null, &ok,
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

#warning "This is rather crappy"
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
#warning "FIXME: do not call kaddressbook"
  KRun::runCommand( "kaddressbook --editor-only --new-contact" );
}

void
AddressesDialog::deleteEntry()
{
}

void
AddressesDialog::cleanEdit()
{
  d->ui->mFilterEdit->clear();
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
        item->setOpen( false );//close to not have too manu entries
      continue;
    }
    if ( item->category() == AddresseeViewItem::Entry ) {
      QString name  = item->addressee().fullEmail();
      if ( !name.contains(txt) )
        item->setVisible( false );
      else {
        if ( item->category() != AddresseeViewItem::Group ) {
          item->parent()->setOpen( true );//open the parents with found entries
        }
        item->setVisible( true );
      }
    }
  }
}

KABC::Addressee::List
AddressesDialog::selectedAddressee( KListView* view ) const
{
  KABC::Addressee::List lst;
  QListViewItemIterator it( view );
  while ( it.current() ) {
    AddresseeViewItem* item = static_cast<AddresseeViewItem*>( it.current() );
    ++it;
    if ( item->isSelected() ) {
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
  }
  return lst;
}

KABC::Addressee::List
AddressesDialog::allAddressee( AddresseeViewItem* parent ) const
{
  KABC::Addressee::List lst;

  if ( !parent ) return KABC::Addressee::List();

  AddresseeViewItem *myChild = static_cast<AddresseeViewItem*>( parent->firstChild() );
  while( myChild ) {
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
    for( KABC::DistributionList::Entry::List::iterator itr = elist.begin();
         itr != elist.end(); ++itr ) {
      addAddresseeToAvailable( (*itr).addressee, parent );
    }
  }
}

}//end namespace KPIM

#include "addressesdialog.moc"
