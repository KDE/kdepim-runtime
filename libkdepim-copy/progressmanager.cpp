/*
  progressmanager.cpp

  This file is part of KDEPIM.

  Author: Till Adam <adam@kde.org> (C) 2004

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

#include <qvaluevector.h>
#include <kdebug.h>

#include <klocale.h>
#include <kstaticdeleter.h>

#include "progressmanager.h"

namespace KPIM {

KPIM::ProgressManager * KPIM::ProgressManager::mInstance = 0;
unsigned int KPIM::ProgressManager::uID = 42;

ProgressItem::ProgressItem(
       ProgressItem* parent, const QString& id,
       const QString& label, const QString& status, bool canBeCanceled,
       bool usesCrypto )
       :mId( id ), mLabel( label ), mStatus( status ), mParent( parent ),
        mCanBeCanceled( canBeCanceled ), mProgress( 0 ), mTotal( 0 ),
        mCompleted( 0 ), mWaitingForKids( false ), mCanceled( false ),
        mUsesCrypto( usesCrypto )
    {}

ProgressItem::~ProgressItem()
{
}

void ProgressItem::setComplete()
{
//   kdDebug(5300) << "ProgressItem::setComplete - " << label() << endl;

   if ( mChildren.isEmpty() ) {
     if ( !mCanceled )
       setProgress( 100 );
     emit progressItemCompleted( this );
     if ( parent() )
       parent()->removeChild( this );
     deleteLater();
   } else {
     mWaitingForKids = true;
   }
}

void ProgressItem::addChild( ProgressItem *kiddo )
{
  mChildren.replace( kiddo, true );
}

void ProgressItem::removeChild( ProgressItem *kiddo )
{
   mChildren.remove( kiddo );
   // in case we were waiting for the last kid to go away, now is the time
   if ( mChildren.count() == 0 && mWaitingForKids ) {
     emit progressItemCompleted( this );
     deleteLater();
   }
}

void ProgressItem::cancel()
{
   if ( mCanceled || !mCanBeCanceled ) return;
   kdDebug(5300) << "ProgressItem::cancel() - " << label() << endl;
   mCanceled = true;
   // Cancel all children.
   QValueList<ProgressItem*> kids = mChildren.keys();
   QValueList<ProgressItem*>::Iterator it( kids.begin() );
   QValueList<ProgressItem*>::Iterator end( kids.end() );
   for ( ; it != end; it++ ) {
     ProgressItem *kid = *it;
     if ( kid->canBeCanceled() )
       kid->cancel();
   }
   setStatus( i18n( "Aborting..." ) );
   emit progressItemCanceled( this );
}


void ProgressItem::setProgress( unsigned int v )
{
   mProgress = v;
   // kdDebug(5300) << "ProgressItem::setProgress(): " << label() << " : " << v << endl;
   emit progressItemProgress( this, mProgress );
}

void ProgressItem::setLabel( const QString& v )
{
  mLabel = v;
  emit progressItemLabel( this, mLabel );
}

void ProgressItem::setStatus( const QString& v )
{
  mStatus = v;
  emit progressItemStatus( this, mStatus );
}

void ProgressItem::setUsesCrypto( bool v )
{
  mUsesCrypto = v;
  emit progressItemUsesCrypto( this, v );
}

// ======================================

ProgressManager::ProgressManager() :QObject() {
  mInstance = this;
}

ProgressManager::~ProgressManager() { mInstance = 0; }
static KStaticDeleter<ProgressManager> progressManagerDeleter;

ProgressManager* ProgressManager::instance()
{
   if ( !mInstance ) {
     progressManagerDeleter.setObject( mInstance, new ProgressManager() );
   }
   return mInstance;
}

ProgressItem* ProgressManager::createProgressItemImpl(
     ProgressItem* parent, const QString& id,
     const QString &label, const QString &status,
     bool cancellable, bool usesCrypto )
{
   ProgressItem *t = 0;
   if ( !mTransactions[ id ] ) {
     t = new ProgressItem ( parent, id, label, status, cancellable, usesCrypto );
     mTransactions.insert( id, t );
     if ( parent ) {
       ProgressItem *p = mTransactions[ parent->id() ];
       if ( p ) {
         p->addChild( t );
       }
     }
     // connect all signals
     connect ( t, SIGNAL( progressItemCompleted(KPIM::ProgressItem*) ),
               this, SLOT( slotTransactionCompleted(KPIM::ProgressItem*) ) );
     connect ( t, SIGNAL( progressItemProgress(KPIM::ProgressItem*, unsigned int) ),
               this, SIGNAL( progressItemProgress(KPIM::ProgressItem*, unsigned int) ) );
     connect ( t, SIGNAL( progressItemAdded(KPIM::ProgressItem*) ),
               this, SIGNAL( progressItemAdded(KPIM::ProgressItem*) ) );
     connect ( t, SIGNAL( progressItemCanceled(KPIM::ProgressItem*) ),
               this, SIGNAL( progressItemCanceled(KPIM::ProgressItem*) ) );
     connect ( t, SIGNAL( progressItemStatus(KPIM::ProgressItem*, const QString&) ),
               this, SIGNAL( progressItemStatus(KPIM::ProgressItem*, const QString&) ) );
     connect ( t, SIGNAL( progressItemLabel(KPIM::ProgressItem*, const QString&) ),
               this, SIGNAL( progressItemLabel(KPIM::ProgressItem*, const QString&) ) );
     connect ( t, SIGNAL( progressItemUsesCrypto(KPIM::ProgressItem*, bool) ),
               this, SIGNAL( progressItemUsesCrypto(KPIM::ProgressItem*, bool) ) );

     emit progressItemAdded( t );
   } else {
     // Hm, is this what makes the most sense?
     t = mTransactions[id];
   }
   return t;
}

ProgressItem* ProgressManager::createProgressItemImpl(
     const QString& parent, const QString &id,
     const QString &label, const QString& status,
     bool canBeCanceled, bool usesCrypto )
{
   ProgressItem * p = mTransactions[parent];
   return createProgressItemImpl( p, id, label, status, canBeCanceled, usesCrypto );
}

void ProgressManager::emitShowProgressDialogImpl()
{
   emit showProgressDialog();
}


// slots

void ProgressManager::slotTransactionCompleted( ProgressItem *item )
{
   mTransactions.remove( item->id() );
   emit progressItemCompleted( item );
}

void ProgressManager::slotStandardCancelHandler( ProgressItem *item )
{
  item->setComplete();
}

ProgressItem* ProgressManager::singleItem() const
{
  ProgressItem *item = 0;
  QDictIterator< ProgressItem > it( mTransactions );
  for ( ; it.current(); ++it ) {
    if ( !(*it)->parent() ) { // if it's a top level one, only those count
      if ( item )
        return 0; // we found more than one
      else
        item = (*it);
    }
  }
  return item;
}

void ProgressManager::slotAbortAll()
{
  QDictIterator< ProgressItem > it( mTransactions );
  for ( ; it.current(); ++it ) {
    it.current()->cancel();
  }
}

} // namespace

#include "progressmanager.moc"
