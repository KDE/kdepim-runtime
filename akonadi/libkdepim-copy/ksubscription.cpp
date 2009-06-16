/*
  This file is part of libkdepim.

  Copyright (C) 2002 Carsten Burghardt <burghardt@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License version 2 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "ksubscription.h"
#include "kaccount.h"

#include <KDebug>
#include <KIconLoader>
#include <KLineEdit>
#include <KLocale>
#include <KSeparator>

#include <QApplication>
#include <QLayout>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>

using namespace KPIM;

//=============================================================================

KGroupInfo::KGroupInfo( const QString &name, const QString &description,
                        bool newGroup, bool subscribed,
                        Status status, const QString &path )
  : name( name ), description( description ),
    newGroup( newGroup ), subscribed( subscribed ),
    status( status ), path( path )
{
}

//-----------------------------------------------------------------------------
bool KGroupInfo::operator== ( const KGroupInfo &gi2 )
{
  return name == gi2.name;
}

//-----------------------------------------------------------------------------
bool KGroupInfo::operator< ( const KGroupInfo &gi2 )
{
  return name < gi2.name;
}

//=============================================================================

GroupItem::GroupItem( QTreeWidget *v, const KGroupInfo &gi, KSubscription *browser,
    bool isCheckItem )
  : QTreeWidgetItem( v, customType ),
    mInfo( gi ), mBrowser( browser ), mIsCheckItem( isCheckItem ),
    mIgnoreStateChange( false )
{
  setText( 0, gi.name );
  if ( isCheckItem ) {
    setCheckState( 0, Qt::Unchecked );
    setFlags( flags() | Qt::ItemIsUserCheckable );
    mLastCheckState = isOn();
  }
  if ( treeWidget()->columnCount() > 1 ) {
    setDescription();
  }

  connect( treeWidget(), SIGNAL( itemChanged ( QTreeWidgetItem *, int ) ),
           this, SLOT( stateChange( QTreeWidgetItem* ) ) );
}

//-----------------------------------------------------------------------------
GroupItem::GroupItem( QTreeWidgetItem *i, const KGroupInfo &gi,
                      KSubscription *browser, bool isCheckItem )
  : QTreeWidgetItem( i, customType ),
    mInfo( gi ), mBrowser( browser ), mIsCheckItem( isCheckItem ),
    mIgnoreStateChange( false )
{
  setText( 0, gi.name );
  if ( isCheckItem ) {
    setCheckState( 0, Qt::Unchecked );
    setFlags( flags() | Qt::ItemIsUserCheckable );
    mLastCheckState = isOn();
  }
  if ( treeWidget()->columnCount() > 1 ) {
    setDescription();
  }
  connect( treeWidget(), SIGNAL( itemChanged ( QTreeWidgetItem *, int ) ),
           this, SLOT( stateChange( QTreeWidgetItem* ) ) );
}

//-----------------------------------------------------------------------------
void GroupItem::setInfo( KGroupInfo info )
{
  mInfo = info;
  setText( 0, mInfo.name );
  if ( treeWidget()->columnCount() > 1 ) {
    setDescription();
  }
}

//-----------------------------------------------------------------------------
void GroupItem::setDescription()
{
  setText( 1, mInfo.description );
}

//-----------------------------------------------------------------------------
void GroupItem::setOn( bool on )
{
  if ( mBrowser->isLoading() ) {
    // set this only if we're loading/creating items
    // otherwise changes are only permanent when the dialog is saved
    mInfo.subscribed = on;
  }
  if ( isCheckItem() ) {
    setCheckState( 0, on ? Qt::Checked : Qt::Unchecked );
  }
}

//------------------------------------------------------------------------------
bool GroupItem::isOn() const
{
  if ( !isCheckItem() )
    return false;
  else
    return checkState( 0 ) == Qt::Checked;
}

//------------------------------------------------------------------------------
void GroupItem::stateChange( QTreeWidgetItem* item )
{
  if ( item != this )
    return;

  // delegate to parent
  if ( !mIgnoreStateChange && mLastCheckState != isOn() ) {
    mBrowser->changeItemState( this, isOn() );
  }

  mLastCheckState = isOn();
}

//------------------------------------------------------------------------------
void GroupItem::setVisible( bool b )
{
  if ( b ) {
    setHidden( !b );
  } else {
    if ( isCheckItem() ) {
      bool setInvisible = true;
      int childIndex = 0;
      while ( QTreeWidgetItem *item = QTreeWidgetItem::child( childIndex++ ) ) {
        if ( !( item->isHidden() ) ) {
          // item has a visible child
          setInvisible = false;
        }
      }
      if ( setInvisible ) {
        setHidden( !b );
      } else {
        treeWidget()->expandItem( this );
      }
    } else {
      // non-checkable item
      typedef QPair<QTreeWidgetItem*,bool> ItemWithVisibility;
      QList< ItemWithVisibility > moveItems;
      int childIndex = 0;
      while ( QTreeWidgetItem *item = QTreeWidgetItem::child( childIndex++ ) ) {
        if ( static_cast<GroupItem*>( item )->isCheckItem() ) {
          // remember the items
          moveItems.append( ItemWithVisibility( item, item->isHidden() ) );
        }
      }
      foreach( const ItemWithVisibility &item, moveItems ) {
        // move the checkitem to top
        QTreeWidgetItem *parent = item.first->parent();
        if ( parent ) {
          parent->removeChild( item.first );
        }
        treeWidget()->insertTopLevelItem( 0, item.first );
        item.first->setHidden( item.second );
      }
      setHidden( true );
    }
  }
}

//=============================================================================

KSubscription::KSubscription( QWidget *parent, const QString &caption,
                              KAccount *acct, KDialog::ButtonCodes buttons,
                              const QString &user1, bool descriptionColumn )
  : KDialog( parent ),
    mAcct( acct )
{
  setCaption( caption );
  setButtons( buttons | Help | Ok | Cancel );
  setDefaultButton( Ok );
  setButtonText( User1, i18n( "Reload &List" ) );
  setButtonText( User2, user1 );
  setModal( true );
  showButtonSeparator( true );

  mLoading = true;
  setAttribute( Qt::WA_DeleteOnClose );

  // create Widgets
  page = new QWidget( this );
  setMainWidget( page );

  QLabel *comment =
    new QLabel(
      "<p>" + i18n( "Manage which mail folders you want to see in your folder view" ) + "</p>",
      page );

  filterEdit = new KLineEdit( page );
  QLabel *l = new QLabel( i18n( "S&earch:" ), page );
  l->setBuddy( filterEdit );
  filterEdit->setClearButtonShown( true );

  // checkboxes
  noTreeCB = new QCheckBox( i18n( "Disable &tree view" ), page );
  noTreeCB->setChecked( false );
  subCB = new QCheckBox( i18n( "&Subscribed only" ), page );
  subCB->setChecked( false );
  newCB = new QCheckBox( i18n( "&New only" ), page );
  newCB->setChecked( false );

  KSeparator *sep = new KSeparator( Qt::Horizontal, page );

  // init the labels
  QFont fnt = font();
  fnt.setBold( true );
  leftLabel = new QLabel( i18n( "Loading..." ), page );
  rightLabel = new QLabel( i18n( "Current changes:" ), page );
  leftLabel->setFont( fnt );
  rightLabel->setFont( fnt );

  // icons
  pmRight = KIcon( "go-next" );
  pmLeft = KIcon( "go-previous" );

  arrowBtn1 = new QPushButton( page );
  arrowBtn1->setEnabled( false );
  arrowBtn2 = new QPushButton( page );
  arrowBtn2->setEnabled( false );
  arrowBtn1->setIcon( pmRight );
  arrowBtn2->setIcon( pmRight );
  arrowBtn1->setFixedSize( 35, 30 );
  arrowBtn2->setFixedSize( 35, 30 );

  // the main listview
  groupView = new QTreeWidget( page );
  groupView->setRootIsDecorated( true );
  groupView->setHeaderLabel( i18nc( "subscription name", "Name" ) );
  groupView->setAllColumnsShowFocus( true );
  groupView->setAlternatingRowColors( true );
  if ( descriptionColumn ) {
    groupView->setHeaderLabel( i18nc( "subscription description", "Description" ) );
  }

  // layout
  QGridLayout *topL = new QGridLayout( page );
  topL->setMargin( 0 );
  topL->setSpacing( KDialog::spacingHint() );

  QHBoxLayout *filterL = new QHBoxLayout();
  filterL->setSpacing( KDialog::spacingHint() );

  QVBoxLayout *arrL = new QVBoxLayout();
  arrL->setSpacing( KDialog::spacingHint() );

  listL = new QGridLayout();
  listL->setSpacing( KDialog::spacingHint() );

  topL->addWidget( comment, 0, 0 );
  topL->addLayout( filterL, 1, 0 );
  topL->addWidget( sep, 2, 0 );
  topL->addLayout( listL, 3, 0 );

  filterL->addWidget( l );
  filterL->addWidget( filterEdit, 1 );
  filterL->addWidget( noTreeCB );
  filterL->addWidget( subCB );
  filterL->addWidget( newCB );

  listL->addWidget( leftLabel, 0, 0 );
  listL->addWidget( rightLabel, 0, 2 );
  listL->addWidget( groupView, 1, 0 );
  listL->addLayout( arrL, 1, 1 );
  listL->setRowStretch( 1, 1 );
  listL->setColumnStretch( 0, 5 );
  listL->setColumnStretch( 2, 2 );

  arrL->addWidget( arrowBtn1, Qt::AlignCenter );
  arrL->addWidget( arrowBtn2, Qt::AlignCenter );

  // listviews
  subView = new QTreeWidget( page );
  subView->setHeaderLabel( i18n( "Subscribe To" ) );
  unsubView = new QTreeWidget( page );
  unsubView->setHeaderLabel( i18n( "Unsubscribe From" ) );

  QVBoxLayout *protL = new QVBoxLayout();
  protL->setSpacing( 3 );

  listL->addLayout( protL, 1, 2 );
  protL->addWidget( subView );
  protL->addWidget( unsubView );

  // disable some widgets as long we're loading
  enableButton( User1, false );
  enableButton( User2, false );
  newCB->setEnabled( false );
  noTreeCB->setEnabled( false );
  subCB->setEnabled( false );

  filterEdit->setFocus();

   // items clicked
  connect( groupView, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ),
           this, SLOT(slotChangeButtonState(QTreeWidgetItem*)));
  connect( subView, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ),
           this, SLOT(slotChangeButtonState(QTreeWidgetItem*)));
  connect( unsubView, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ),
           this, SLOT(slotChangeButtonState(QTreeWidgetItem*)));

  // connect buttons
  connect( arrowBtn1, SIGNAL(clicked()), SLOT(slotButton1()));
  connect( arrowBtn2, SIGNAL(clicked()), SLOT(slotButton2()));
  connect( this, SIGNAL(user1Clicked()), SLOT(slotLoadFolders()));

  // connect checkboxes
  connect( subCB, SIGNAL(clicked()), SLOT(slotCBToggled()));
  connect( newCB, SIGNAL(clicked()), SLOT(slotCBToggled()));
  connect( noTreeCB, SIGNAL(clicked()), SLOT(slotCBToggled()));

  // connect textfield
  connect( filterEdit, SIGNAL(textChanged(const QString&)),
           SLOT(slotFilterTextChanged(const QString&)));

  // update status
  connect( this, SIGNAL(listChanged()), SLOT(slotUpdateStatusLabel()));
}

//-----------------------------------------------------------------------------
KSubscription::~KSubscription()
{
}

//-----------------------------------------------------------------------------
void KSubscription::setStartItem( const KGroupInfo &info )
{
  QTreeWidgetItemIterator it( groupView );

  for ( ; *it; ++it ) {
    if ( static_cast<GroupItem*>( *it )->info() == info ) {
      ( *it )->setSelected( true );
      groupView->expandItem( *it );
    }
  }
}

//-----------------------------------------------------------------------------
void KSubscription::removeListItem( QTreeWidget *view, const KGroupInfo &gi )
{
  if ( !view ) {
    return;
  }
  QTreeWidgetItemIterator it( view );

  for ( ; *it; ++it ) {
    if ( static_cast<GroupItem*>( *it )->info() == gi ) {

      // Delete the item
      QTreeWidgetItem *itemToDelete = *it;
      QTreeWidgetItem *itemParent = itemToDelete->parent();
      if ( itemParent )
        delete itemParent->takeChild( itemParent->indexOfChild( itemToDelete ) );
      else {
        QTreeWidget *treeWidget = itemToDelete->treeWidget();
        delete treeWidget->takeTopLevelItem( treeWidget->indexOfTopLevelItem( itemToDelete ) );
      }
      break;
    }
  }
  if ( view == groupView ) {
    emit listChanged();
  }
}

//-----------------------------------------------------------------------------
QTreeWidgetItem* KSubscription::getListItem( QTreeWidget *view, const KGroupInfo &gi )
{
  if ( !view ) {
    return 0;
  }
  QTreeWidgetItemIterator it( view );

  for ( ; *it; ++it ) {
    if ( static_cast<GroupItem*>( *it )->info() == gi ) {
      return *it;
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
bool KSubscription::itemInListView( QTreeWidget *view, const KGroupInfo &gi )
{
  if ( !view ) {
    return false;
  }
  QTreeWidgetItemIterator it( view );

  for ( ; *it; ++it ) {
    if ( static_cast<GroupItem*>( *it )->info() == gi ) {
      return true;
    }
  }

  return false;
}

//------------------------------------------------------------------------------
void KSubscription::setDirectionButton1( Direction dir )
{
  mDirButton1 = dir;
  if ( dir == Left ) {
    arrowBtn1->setIcon( pmLeft );
  } else {
    arrowBtn1->setIcon( pmRight );
  }
}

//------------------------------------------------------------------------------
void KSubscription::setDirectionButton2( Direction dir )
{
  mDirButton2 = dir;
  if ( dir == Left ) {
    arrowBtn2->setIcon( pmLeft );
  } else {
    arrowBtn2->setIcon( pmRight );
  }
}

//------------------------------------------------------------------------------
void KSubscription::changeItemState( GroupItem *item, bool on )
{
  // is this a checkable item
  if ( !item->isCheckItem() ) {
    return;
  }

  // if we're currently loading the items ignore changes
  if ( mLoading ) {
    return;
  }
  if ( on ) {
    if ( !itemInListView( unsubView, item->info() ) ) {
      QTreeWidgetItem *p = item->QTreeWidgetItem::parent();
      while ( p ) {
        // make sure all parents are subscribed
        GroupItem *pi = static_cast<GroupItem*>( p );
        if ( pi->isCheckItem() && !pi->isOn() ) {
          pi->setIgnoreStateChange( true );
          pi->setOn( true );
          pi->setIgnoreStateChange( false );
          new GroupItem( subView, pi->info(), this );
        }
        p = p->parent();
      }
      new GroupItem( subView, item->info(), this );
    }
    // eventually remove it from the other listview
    removeListItem( unsubView, item->info() );
  } else {
    if ( !itemInListView( subView, item->info() ) ) {
      new GroupItem( unsubView, item->info(), this );
    }
    // eventually remove it from the other listview
    removeListItem( subView, item->info() );
  }
  // update the buttons
  slotChangeButtonState( item );
}

//------------------------------------------------------------------------------
void KSubscription::filterChanged( QTreeWidgetItem *item, const QString &text )
{
  if ( !item && groupView ) {
    item = groupView->topLevelItem( 0 );
  }
  if ( !item ) {
    return;
  }

  QTreeWidgetItem *parent = item->parent();
  if ( !parent )
    parent = groupView->invisibleRootItem();
  Q_ASSERT( parent );
  int childIndex = parent->indexOfChild( item );
  while( ( item = parent->child( childIndex++ ) ) )
  {
    if ( item->childCount() > 0 ) {
      // recursive descend
      filterChanged( item->child( 0 ), text );
    }

    GroupItem *gr = static_cast<GroupItem*>( item );
    if ( subCB->isChecked() || newCB->isChecked() || !text.isEmpty() || noTreeCB->isChecked() ) {
      // set it invisible
      if ( subCB->isChecked() &&
           ( !gr->isCheckItem() || ( gr->isCheckItem() && !gr->info().subscribed ) ) ) {
        // only subscribed
        gr->setVisible( false );
        continue;
      }
      if ( newCB->isChecked() &&
           ( !gr->isCheckItem() || ( gr->isCheckItem() && !gr->info().newGroup ) ) ) {
        // only new
        gr->setVisible( false );
        continue;
      }
      if ( !text.isEmpty() && gr->text( 0 ).indexOf( text, 0, Qt::CaseInsensitive ) == -1 ) {
        // searchfield
        gr->setVisible( false );
        continue;
      }
      if ( noTreeCB->isChecked() && !gr->isCheckItem() ) {
        // disable treeview
        gr->setVisible( false );
        continue;
      }

      gr->setVisible( true );

    } else {
      gr->setVisible( true );
    }
  }
}

//------------------------------------------------------------------------------
int KSubscription::activeItemCount()
{
  QTreeWidgetItemIterator it( groupView );

  int count = 0;
  for ( ; *it; ++it ) {
    if ( static_cast<GroupItem*>( *it )->isCheckItem() &&
         !( ( *it )->isHidden() ) ) {
      count++;
    }
  }

  return count;
}

//------------------------------------------------------------------------------
void KSubscription::restoreOriginalParent()
{
  QList<QTreeWidgetItem*> move;
  QTreeWidgetItemIterator it( groupView );
  for ( ; *it; ++it ) {
    QTreeWidgetItem *origParent =
      static_cast<GroupItem*>( *it )->originalParent();
    if ( origParent && origParent != ( *it )->parent() ) {
      // remember this to avoid messing up the iterator
      move.append( *it );
    }
  }
  foreach( QTreeWidgetItem *item, move ) {
    // restore the original parent
    QTreeWidgetItem *origParent =
      static_cast<GroupItem*>( item )->originalParent();
    groupView->takeTopLevelItem( groupView->indexOfTopLevelItem( item ) );
    origParent->insertChild( 0, item );
  }
}

//-----------------------------------------------------------------------------
void KSubscription::saveOpenStates()
{
  QTreeWidgetItemIterator it( groupView );

  for ( ; *it; ++it ) {
    static_cast<GroupItem*>( *it )->setLastOpenState( groupView->isItemExpanded( *it ) );
  }
}

//-----------------------------------------------------------------------------
void KSubscription::restoreOpenStates()
{
  QTreeWidgetItemIterator it( groupView );

  for ( ; *it; ++it ) {
    bool wasExpanded = static_cast<GroupItem*>( *it )->lastOpenState();
    if ( wasExpanded )
      groupView->expandItem( *it );
    else
      groupView->collapseItem( *it );
  }
}

//-----------------------------------------------------------------------------
void KSubscription::slotLoadingComplete()
{
  mLoading = false;

  enableButton( User1, true );
  enableButton( User2, true );
  newCB->setEnabled( true );
  noTreeCB->setEnabled( true );
  subCB->setEnabled( true );

  // remember the correct parent
  QTreeWidgetItemIterator it( groupView );
  for ( ; *it; ++it ) {
    static_cast<GroupItem*>( *it )->setOriginalParent( ( *it )->parent() );
  }

  emit listChanged();
}

//------------------------------------------------------------------------------
void KSubscription::slotChangeButtonState( QTreeWidgetItem *item )
{
  if ( !item ||
       ( item->treeWidget() == groupView && !static_cast<GroupItem*>(item)->isCheckItem() ) ) {
    // disable and return
    arrowBtn1->setEnabled( false );
    arrowBtn2->setEnabled( false );
    return;
  }
  // set the direction of the buttons and enable/disable them
  QTreeWidget *currentView = item->treeWidget();
  if ( currentView == groupView ) {
    setDirectionButton1( Right );
    setDirectionButton2( Right );
    if ( static_cast<GroupItem*>(item)->isOn() ) {
      // already subscribed
      arrowBtn1->setEnabled( false );
      arrowBtn2->setEnabled( true );
    } else {
      // unsubscribed
      arrowBtn1->setEnabled( true );
      arrowBtn2->setEnabled( false );
    }
  } else if ( currentView == subView ) {
    // undo possible
    setDirectionButton1( Left );

    arrowBtn1->setEnabled( true );
    arrowBtn2->setEnabled( false );
  } else if ( currentView == unsubView ) {
    // undo possible
    setDirectionButton2( Left );

    arrowBtn1->setEnabled( false );
    arrowBtn2->setEnabled( true );
  }
}

//------------------------------------------------------------------------------
void KSubscription::slotButton1()
{
  if ( mDirButton1 == Right ) {
    if ( groupView->currentItem() &&
         static_cast<GroupItem*>( groupView->currentItem() )->isCheckItem() ) {
      // activate
      GroupItem *item = static_cast<GroupItem*>( groupView->currentItem() );
      item->setOn( true );
    }
  } else {
    if ( subView->currentItem() ) {
      GroupItem *item = static_cast<GroupItem*>( subView->currentItem() );
      // get the corresponding item from the groupView
      QTreeWidgetItem *listitem = getListItem( groupView, item->info() );
      if ( listitem ) {
        // deactivate
        GroupItem *chk = static_cast<GroupItem*>( listitem );
        chk->setOn( false );
      }
    }
  }
}

//------------------------------------------------------------------------------
void KSubscription::slotButton2()
{
  if ( mDirButton2 == Right ) {
    if ( groupView->currentItem() &&
         static_cast<GroupItem*>( groupView->currentItem() )->isCheckItem() ) {
      // deactivate
      GroupItem *item = static_cast<GroupItem*>( groupView->currentItem() );
      item->setOn( false );
    }
  } else {
    if ( unsubView->currentItem() ) {
      GroupItem *item = static_cast<GroupItem*>( unsubView->currentItem() );
      // get the corresponding item from the groupView
      QTreeWidgetItem *listitem = getListItem( groupView, item->info() );
      if ( listitem ) {
        // activate
        GroupItem *chk = static_cast<GroupItem*>( listitem );
        chk->setOn( true );
      }
    }
  }
}

//------------------------------------------------------------------------------
void KSubscription::slotCBToggled()
{
  if ( !noTreeCB->isChecked() && !newCB->isChecked() && !subCB->isChecked() ) {
    restoreOriginalParent();
  }
  // set items {in}visible
  filterChanged( groupView->topLevelItem( 0 ) );
  emit listChanged();
}

//------------------------------------------------------------------------------
void KSubscription::slotFilterTextChanged( const QString &text )
{
  // remember is the items are open
  if ( mLastText.isEmpty() ) {
    saveOpenStates();
  }

  if ( !mLastText.isEmpty() && text.length() < mLastText.length() ) {
    // reset
    restoreOriginalParent();
    QTreeWidgetItemIterator it( groupView );
    for ( ; *it; ++it ) {
      ( *it )->setHidden( false );
    }
  }
  // set items {in}visible
  filterChanged( groupView->topLevelItem( 0 ), text );
  // restore the open-states
  if ( text.isEmpty() ) {
    restoreOpenStates();
  }

  emit listChanged();
  mLastText = text;
}

//------------------------------------------------------------------------------
void KSubscription::slotUpdateStatusLabel()
{
  QString text;
  if ( mLoading ) {
    text = i18np( "Loading... (1 matching)", "Loading... (%1 matching)", activeItemCount() );
  } else {
    text = i18np( "%2: (1 matching)", "%2: (%1 matching)", activeItemCount(), account()->name() );
  }

  leftLabel->setText( text );
}

//------------------------------------------------------------------------------
void KSubscription::slotLoadFolders()
{
  enableButton( User1, false );
  mLoading = true;
  subView->clear();
  unsubView->clear();
  groupView->clear();
}
