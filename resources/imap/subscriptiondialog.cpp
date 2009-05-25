/*  -*- c++ -*-
    subscriptiondialog.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (C) 2002 Carsten Burghardt <burghardt@kde.org>
    Copyright (C) 2009 Kevin Ottens <ervin@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "subscriptiondialog.h"

#include <QtCore/QTimer>

#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>

#include <kimap/session.h>
#include <kimap/listjob.h>
#include <kimap/unsubscribejob.h>
#include <kimap/subscribejob.h>

SubscriptionDialogBase::SubscriptionDialogBase( QWidget *parent, const QString &caption,
    KAccount *acct, const QString &startPath )
  : KSubscription( parent, caption, acct, User1, QString(), false ),
    mStartPath( startPath ), mSubscribed( false ), mForceSubscriptionEnable( false )
{
  // hide unneeded checkboxes
  hideTreeCheckbox();
  hideNewOnlyCheckbox();

  // reload-list button
  connect(this, SIGNAL(user1Clicked()), SLOT(slotLoadFolders()));

  // get the folders, delayed execution style, otherwise there's bother
  // with virtuals from ctors and whatnot
  QTimer::singleShot(0, this, SLOT(slotLoadFolders()));
}

//------------------------------------------------------------------------------
void SubscriptionDialogBase::slotListDirectory( KJob *job )
{
  KIMAP::ListJob *list = static_cast<KIMAP::ListJob*>( job );

  QList<KIMAP::MailBoxDescriptor> mailBoxes = list->mailBoxes();

  foreach ( const KIMAP::MailBoxDescriptor &mailBox, mailBoxes ) {
    mDelimiters << mailBox.separator;
    mFolderPaths << mailBox.name;
    mFolderNames << mailBox.name.split(mailBox.separator).last();
    mFolderSelectable << !list->flags()[mailBox].contains("\\NoSelect");
  }

  mOnlySubscribed = !list->isIncludeUnsubscribed();
  mCount = 0;

  processFolderListing();
}

void SubscriptionDialogBase::slotButtonClicked( int button )
{
  if ( button == KDialog::Ok ) {
    if ( doSave() ) {
      accept();
    }
  }
  else {
    KDialog::slotButtonClicked( button );
  }
}

void SubscriptionDialogBase::moveChildrenToNewParent( GroupItem *oldItem, GroupItem *item  )
{
  if ( !oldItem || !item ) return;

  QList<QTreeWidgetItem*> itemsToMove;
  int childIndex = 0;
  while ( QTreeWidgetItem* curItem = oldItem->QTreeWidgetItem::child( childIndex++ ) ) {
    itemsToMove.append( curItem );
    }

  foreach( QTreeWidgetItem *cur, itemsToMove ) {
    QTreeWidgetItem *taken = oldItem->takeChild( oldItem->indexOfChild( cur ) );
    Q_ASSERT( taken );
    item->QTreeWidgetItem::insertChild( 0, taken );
    if ( taken->isSelected() ) // we have new parents so open them
      folderTree()->scrollToItem( taken );
  }

  // Delete the old item
  QTreeWidgetItem *oldItemParent = oldItem->QTreeWidgetItem::parent();
  if ( oldItemParent )
    delete oldItemParent->takeChild( oldItemParent->indexOfChild( oldItem ) );
  else {
    QTreeWidget *treeWidget = oldItem->treeWidget();
    delete treeWidget->takeTopLevelItem( treeWidget->indexOfTopLevelItem( oldItem ) );
  }
  itemsToMove.clear();
}

void SubscriptionDialogBase::createListViewItem( int i )
{
  GroupItem *item = 0;
  GroupItem *parent = 0;

  // get the parent
  GroupItem *oldItem = 0;
  QString parentPath;
  findParentItem( mFolderNames[i], mFolderPaths[i], parentPath, &parent, &oldItem );

  if (!parent && !parentPath.isEmpty())
  {
    // the parent is not available and it's no root-item
    // this happens when the folders do not arrive in hierarchical order
    // so we create each parent in advance
    QStringList folders = parentPath.split( mDelimiters[i], QString::SkipEmptyParts );
    uint i = 0;
    for ( QStringList::Iterator it = folders.begin(); it != folders.end(); ++it )
    {
      KGroupInfo info(*it);
      info.subscribed = false;

      QStringList tmpPath;
      for ( uint j = 0; j <= i; ++j )
        tmpPath << folders[j];
      QString path = tmpPath.join(mDelimiters[i]);
      info.path = path;
      item = mItemDict[path];
      // as these items are "dummies" we create them non-checkable
      if (!item)
      {
        if (parent) {
          item = new GroupItem(parent, info, this, false);
        }
        else {
          item = new GroupItem(folderTree(), info, this, false);
        }
        mItemDict.insert(info.path, item);
      }

      parent = item;
      ++i;
    } // folders
  } // parent

  KGroupInfo info(mFolderNames[i]);
  if (mFolderNames[i].toUpper() == "INBOX" &&
      mFolderPaths[i] == "INBOX")
    info.name = "Inbox";
  info.subscribed = false;
  info.path = mFolderPaths[i];
  // only checkable when the folder is selectable
  bool checkable = mFolderSelectable[i];
  // create a new item
  if ( parent ) {
    item = new GroupItem(parent, info, this, checkable);
  }
  else {
    item = new GroupItem(folderTree(), info, this, checkable);
  }

  if (oldItem) // remove old item
    mItemDict.remove(info.path);

  mItemDict.insert(info.path, item);
  if (oldItem)
    moveChildrenToNewParent( oldItem, item );

  // select the start item
  if ( mFolderPaths[i] == mStartPath )
  {
    item->setSelected( true );
    folderTree()->scrollToItem( item );
  }
}



//------------------------------------------------------------------------------
void SubscriptionDialogBase::findParentItem( QString &name, QString &path, QString &parentPath,
    GroupItem **parent, GroupItem **oldItem )
{
  // remove the name (and the separator) from the path to get the parent path
  int start = path.length() - (name.length()+1);
  int length = name.length()+1;
  if (start < 0) start = 0;
  parentPath = path;
  parentPath.remove(start, length);

  // find the parent by it's path
  QMap<QString, GroupItem*>::Iterator it = mItemDict.find( parentPath );
  if ( it != mItemDict.end() )
    *parent = ( *it );

  // check if the item already exists
  it = mItemDict.find( path );
  if ( it != mItemDict.end() )
    *oldItem = ( *it );
}

//------------------------------------------------------------------------------
void SubscriptionDialogBase::slotLoadFolders()
{
  ImapAccount* ai = static_cast<ImapAccount*>(account());
  // we need a connection
  if ( !ai->session() || ai->session()->state() != KIMAP::Session::Authenticated )
  {
    kWarning() <<"SubscriptionDialog - got no connection";
    return;
  }

  // clear the views
  KSubscription::slotLoadFolders();
  mItemDict.clear();
  mSubscribed = false;
  mLoading = true;

  // first step is to load a list of all available folders and create listview
  // items for them
  listAllAvailableAndCreateItems();
}

//------------------------------------------------------------------------------
void SubscriptionDialogBase::processNext()
{
  mDelimiters.clear();
  mFolderNames.clear();
  mFolderPaths.clear();
  mFolderSelectable.clear();

  ImapAccount* ai = static_cast<ImapAccount*>(account());

  KIMAP::ListJob *list = new KIMAP::ListJob( ai->session() );
  list->setIncludeUnsubscribed( !mSubscribed );
  connect( list, SIGNAL( result(KJob*) ), this, SLOT( slotListDirectory(KJob*) ) );
  list->start();
}

void SubscriptionDialogBase::loadingComplete()
{
  slotLoadingComplete();
}


//------------------------------------------------------------------------------
// implementation for server side subscription
//------------------------------------------------------------------------------

SubscriptionDialog::SubscriptionDialog( QWidget *parent, const QString &caption,
    KAccount *acct, const QString & startPath )
  : SubscriptionDialogBase( parent, caption, acct, startPath )
{
}

/* virtual */
SubscriptionDialog::~SubscriptionDialog()
{
}

/* virtual */
void SubscriptionDialog::listAllAvailableAndCreateItems()
{
  processNext();
}

//------------------------------------------------------------------------------
void SubscriptionDialogBase::slotConnectionSuccess()
{
  slotLoadFolders();
}

bool SubscriptionDialogBase::checkIfSubscriptionsEnabled()
{
  ImapAccount *account = static_cast<ImapAccount*>(mAcct);
  if( !account )
    return true;
  if( subscriptionOptionEnabled( account ) )
    return true;

  int result = KMessageBox::questionYesNoCancel( this,
      subscriptionOptionQuestion( account->server() ),
      i18n("Enable Subscriptions?"), KGuiItem( i18n("Enable") ), KGuiItem( i18n("Do Not Enable") ) );

  switch(result) {
    case KMessageBox::Yes:
        mForceSubscriptionEnable = true;
        break;
    case KMessageBox::No:
        break;
    case KMessageBox::Cancel:
        return false;
        break;
  }

  return true;
}

// =======
/* virtual */
void SubscriptionDialog::processFolderListing()
{
  processItems();
}

/* virtual */
bool SubscriptionDialog::doSave()
{
  if ( !checkIfSubscriptionsEnabled() )
    return false;

  ImapAccount *ai = static_cast<ImapAccount*>(account());

  // subscribe
  QTreeWidgetItemIterator it(subView);
  for ( ; *it; ++it)
  {
    KIMAP::SubscribeJob *subscribe = new KIMAP::SubscribeJob( ai->session() );
    subscribe->setMailBox(
      static_cast<GroupItem*>(*it)->info().path
    );
    subscribe->exec();
  }

  // unsubscribe
  QTreeWidgetItemIterator it2(unsubView);
  for ( ; *it2; ++it2)
  {
    KIMAP::UnsubscribeJob *unsubscribe = new KIMAP::UnsubscribeJob( ai->session() );
    unsubscribe->setMailBox(
      static_cast<GroupItem*>(*it2)->info().path
    );
    unsubscribe->exec();
  }

  if ( mForceSubscriptionEnable ) {
    ai->setSubscriptionEnabled( true );
  }

  return true;
}

bool SubscriptionDialog::subscriptionOptionEnabled( const ImapAccount *account ) const
{
  return account->isSubscriptionEnabled();
}

QString SubscriptionDialog::subscriptionOptionQuestion( const QString &accountName ) const
{
  return i18nc( "@info", "Currently subscriptions are not used for server <resource>%1</resource>.<nl/>"
                         "\nDo you want to enable subscriptions?", accountName );
}

void SubscriptionDialog::processItems()
{
  bool onlySubscribed = mOnlySubscribed;
  uint done = 0;
  for (int i = mCount; i < mFolderNames.count(); ++i)
  {
    // give the dialog a chance to repaint
    if (done == 1000)
    {
      emit listChanged();
      QTimer::singleShot(0, this, SLOT(processItems()));
      return;
    }
    ++mCount;
    ++done;
    if (!onlySubscribed && mFolderPaths.size() > 0)
    {
      createListViewItem( i );
    } else if (onlySubscribed)
    {
      // find the item
      if ( mItemDict[mFolderPaths[i]] )
      {
        GroupItem* item = mItemDict[mFolderPaths[i]];
        item->setOn( true );
      }
    }
  }

  if ( !mSubscribed ) {
    mSubscribed = true;
    processNext();
  } else {
    loadingComplete();
  }
}

#include "subscriptiondialog.moc"
