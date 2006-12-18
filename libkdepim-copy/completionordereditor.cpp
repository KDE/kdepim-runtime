/** -*- c++ -*-
 * completionordereditor.cpp
 *
 *  Copyright (c) 2004 David Faure <faure@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */

#include "completionordereditor.h"
#include "completionordereditor_p.h"
#include "ldapclient.h"
#include "resourceabc.h"

#include <QtDBus/QDBusConnection>

#include <kabc/stdaddressbook.h>
#include <kabc/resource.h>

#include <kdebug.h>
#include <klocale.h>
#include <kicon.h>
#include <k3listview.h>
#include <kpushbutton.h>

#include <khbox.h>
#include <kvbox.h>
#include <q3header.h>
#include <QToolButton>
#include <kapplication.h>

/*

Several items are used in addresseelineedit's completion object:
  LDAP servers, KABC resources (imap and non-imap), Recent addresses (in kmail only).

The default completion weights are as follow:
  LDAP: 50, 49, 48 etc.          (see ldapclient.cpp)
  KABC non-imap resources: 60    (see addresseelineedit.cpp and SimpleCompletionItem here)
  Distribution lists: 60         (see addresseelineedit.cpp and SimpleCompletionItem here)
  KABC imap resources: 80        (see kresources/imap/kabc/resourceimap.cpp)
  Recent addresses (kmail) : 120 (see kmail/kmcomposewin.cpp)

This dialog allows to change those weights, by showing one item per:
 - LDAP server
 - KABC non-imap resource
 - KABC imap subresource
 plus one item for Distribution Lists.

 Maybe 'recent addresses' should be configurable too, but first it might
 be better to add support for them in korganizer too.

*/

using namespace KPIM;

CompletionOrderEditorAdaptor::CompletionOrderEditorAdaptor(QObject *parent)
  : QDBusAbstractAdaptor(parent)
{
  setAutoRelaySignals(true);
}

int CompletionItemList::compareItems( Q3PtrCollection::Item s1, Q3PtrCollection::Item s2 )
{
  int w1 = ( (CompletionItem*)s1 )->completionWeight();
  int w2 = ( (CompletionItem*)s2 )->completionWeight();
  // s1 < s2 if it has a higher completion value, i.e. w1 > w2.
  return w2 - w1;
}

class LDAPCompletionItem : public CompletionItem
{
public:
  LDAPCompletionItem( LdapClient* ldapClient ) : mLdapClient( ldapClient ) {}
  virtual QString label() const { return i18n( "LDAP server %1", mLdapClient->server().host() ); }
  virtual int completionWeight() const { return mLdapClient->completionWeight(); }
  virtual void save( CompletionOrderEditor* );
protected:
  virtual void setCompletionWeight( int weight ) { mWeight = weight; }
private:
  LdapClient* mLdapClient;
  int mWeight;
};

void LDAPCompletionItem::save( CompletionOrderEditor* )
{
  KConfig config( "kabldaprc" );
  config.setGroup( "LDAP" );
  config.writeEntry( QString( "SelectedCompletionWeight%1" ).arg( mLdapClient->clientNumber() ),
                     mWeight );
  config.sync();
}

// A simple item saved into kpimcompletionorder (no subresources, just name/identifier/weight)
class SimpleCompletionItem : public CompletionItem
{
public:
  SimpleCompletionItem( CompletionOrderEditor* editor, const QString& label, const QString& identifier )
    : mLabel( label ), mIdentifier( identifier ) {
      KConfigGroup group( editor->configFile(), "CompletionWeights" );
      mWeight = group.readEntry( mIdentifier, 60 );
    }
  virtual QString label() const { return mLabel; }
  virtual int completionWeight() const { return mWeight; }
  virtual void save( CompletionOrderEditor* );
protected:
  virtual void setCompletionWeight( int weight ) { mWeight = weight; }
private:
  QString mLabel, mIdentifier;
  int mWeight;
};

void SimpleCompletionItem::save( CompletionOrderEditor* editor )
{
  // Maybe KABC::Resource could have a completionWeight setting (for readConfig/writeConfig)
  // But for kdelibs-3.2 compat purposes I can't do that.
  KConfigGroup group( editor->configFile(), "CompletionWeights" );
  group.writeEntry( mIdentifier, mWeight );
}

// An imap subresource for kabc
class KABCImapSubResCompletionItem : public CompletionItem
{
public:
  KABCImapSubResCompletionItem( ResourceABC* resource, const QString& subResource )
    : mResource( resource ), mSubResource( subResource ), mWeight( completionWeight() ) {}
  virtual QString label() const {
    return QString( "%1 %2" ).arg( mResource->resourceName() ).arg( mResource->subresourceLabel( mSubResource ) );
  }
  virtual int completionWeight() const {
    return mResource->subresourceCompletionWeight( mSubResource );
  }
  virtual void setCompletionWeight( int weight ) {
    mWeight = weight;
  }
  virtual void save( CompletionOrderEditor* ) {
    mResource->setSubresourceCompletionWeight( mSubResource, mWeight );
  }
private:
  ResourceABC* mResource;
  QString mSubResource;
  int mWeight;
};

/////////

class CompletionViewItem : public Q3ListViewItem
{
public:
  CompletionViewItem( Q3ListView* lv, CompletionItem* item )
    : Q3ListViewItem( lv, lv->lastItem(), item->label() ), mItem( item ) {}
  CompletionItem* item() const { return mItem; }
  void setItem( CompletionItem* i ) { mItem = i; setText( 0, mItem->label() ); }

private:
  CompletionItem* mItem;
};

CompletionOrderEditor::CompletionOrderEditor( KPIM::LdapSearch* ldapSearch,
                                              QWidget* parent )
  : KDialog( parent ), mConfig( "kpimcompletionorder" ), mDirty( false )
{
  setCaption( i18n( "Edit Completion Order" ) );
  setButtons( Ok|Cancel );
  setDefaultButton( Ok );
  setModal( true );
  showButtonSeparator( true );
  new CompletionOrderEditorAdaptor( this );
  QDBusConnection::sessionBus().registerObject("/", this, QDBusConnection::ExportAdaptors);
  mItems.setAutoDelete( true );
  // The first step is to gather all the data, creating CompletionItem objects
  QList< LdapClient* > ldapClients = ldapSearch->clients();
  for( QList<LdapClient*>::const_iterator it = ldapClients.begin(); it != ldapClients.end(); ++it ) {
    //kDebug(5300) << "LDAP: host " << (*it)->host() << " weight " << (*it)->completionWeight() << endl;
    mItems.append( new LDAPCompletionItem( *it ) );
  }
  KABC::AddressBook *addressBook = KABC::StdAddressBook::self( true );
  QList<KABC::Resource*> resources = addressBook->resources();
  QListIterator<KABC::Resource*> resit( resources );
  while ( resit.hasNext() ) {
    KABC::Resource *resource = resit.next();
    //kDebug(5300) << "KABC Resource: " << (*resit)->className() << endl;
    ResourceABC* res = dynamic_cast<ResourceABC *>( resource  );
    if ( res ) { // IMAP KABC resource
      const QStringList subresources = res->subresources();
      for( QStringList::const_iterator it = subresources.begin(); it != subresources.end(); ++it ) {
        mItems.append( new KABCImapSubResCompletionItem( res, *it ) );
      }
    } else { // non-IMAP KABC resource
      mItems.append( new SimpleCompletionItem( this, resource->resourceName(),
                                               resource->identifier() ) );
    }
  }

#ifndef KDEPIM_NEW_DISTRLISTS // new distr lists are normal contact, so no separate item if using them
  // Add an item for distribution lists
  mItems.append( new SimpleCompletionItem( this, i18n( "Distribution Lists" ), "DistributionLists" ) );
#endif

  // Now sort the items, then create the GUI
  mItems.sort();

  KHBox* page = new KHBox( this );
  setMainWidget( page );
  mListView = new K3ListView( page );
  mListView->setSorting( -1 );
  mListView->addColumn( QString() );
  mListView->header()->hide();

  for( Q3PtrListIterator<CompletionItem> compit( mItems ); *compit; ++compit ) {
    new CompletionViewItem( mListView, *compit );
    kDebug(5300) << "  " << (*compit)->label() << " " << (*compit)->completionWeight() << endl;
  }

  KVBox* upDownBox = new KVBox( page );
  mUpButton = new KPushButton( upDownBox );
  mUpButton->setObjectName( "mUpButton" );
  mUpButton->setIcon( KIcon("up") );
  mUpButton->setEnabled( false ); // b/c no item is selected yet
  mUpButton->setFocusPolicy( Qt::StrongFocus );

  mDownButton = new KPushButton( upDownBox );
  mDownButton->setObjectName( "mDownButton" );
  mDownButton->setIcon( KIcon("down") );
  mDownButton->setEnabled( false ); // b/c no item is selected yet
  mDownButton->setFocusPolicy( Qt::StrongFocus );

  QWidget* spacer = new QWidget( upDownBox );
  upDownBox->setStretchFactor( spacer, 100 );

  connect( mListView, SIGNAL( selectionChanged( Q3ListViewItem* ) ),
           SLOT( slotSelectionChanged( Q3ListViewItem* ) ) );
  connect( mUpButton, SIGNAL( clicked() ), this, SLOT( slotMoveUp() ) );
  connect( mDownButton, SIGNAL( clicked() ), this, SLOT( slotMoveDown() ) );
}

CompletionOrderEditor::~CompletionOrderEditor()
{
}

void CompletionOrderEditor::slotSelectionChanged( Q3ListViewItem *item )
{
  mDownButton->setEnabled( item && item->itemBelow() );
  mUpButton->setEnabled( item && item->itemAbove() );
}

static void swapItems( CompletionViewItem *one, CompletionViewItem *other )
{
  CompletionItem* i = one->item();
  one->setItem( other->item() );
  other->setItem( i );
}

void CompletionOrderEditor::slotMoveUp()
{
  CompletionViewItem *item = static_cast<CompletionViewItem *>( mListView->selectedItem() );
  if ( !item ) return;
  CompletionViewItem *above = static_cast<CompletionViewItem *>( item->itemAbove() );
  if ( !above ) return;
  swapItems( item, above );
  mListView->setCurrentItem( above );
  mListView->setSelected( above, true );
  mDirty = true;
}

void CompletionOrderEditor::slotMoveDown()
{
  CompletionViewItem *item = static_cast<CompletionViewItem *>( mListView->selectedItem() );
  if ( !item ) return;
  CompletionViewItem *below = static_cast<CompletionViewItem *>( item->itemBelow() );
  if ( !below ) return;
  swapItems( item, below );
  mListView->setCurrentItem( below );
  mListView->setSelected( below, true );
  mDirty = true;
}

void CompletionOrderEditor::slotOk()
{
  if ( mDirty ) {
    int w = 100;
    for ( Q3ListViewItem* it = mListView->firstChild(); it; it = it->nextSibling() ) {
      CompletionViewItem *item = static_cast<CompletionViewItem *>( it );
      item->item()->setCompletionWeight( w );
      item->item()->save( this );
      kDebug(5300) << "slotOk:   " << item->item()->label() << " " << w << endl;
      --w;
    }
    emit completionOrderChanged();
  }
  KDialog::accept();
}

#include "completionordereditor_p.moc"
#include "completionordereditor.moc"
