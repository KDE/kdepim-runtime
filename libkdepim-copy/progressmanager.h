/*
  progressmanager.h

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

#ifndef __KPIM_PROGRESSMANAGER_H__
#define __KPIM_PROGRESSMANAGER_H__

#include <qobject.h>
#include <qdict.h>
#include <qstring.h>

namespace KPIM {

class ProgressItem;
class ProgressManager;
typedef QMap<ProgressItem*, bool> ProgressItemMap;

class ProgressItem : public QObject
{
  Q_OBJECT
  friend class ProgressManager;
  friend class QDict< ProgressItem >; // so it can be deleted from dicts

  public:

    /**
     * @return The id string which uniquely identifies the operation
     *         represented by this item.
     */
    const QString& id() const { return mId; }

    /**
     * @return The parent item of this one, if there is one.
     */
    ProgressItem *parent() const { return mParent; }

    /**
     * @return The user visible string to be used to represent this item.
     */
    const QString& label() const { return mLabel; }

    /**
     * @param v Set the user visible string identifying this item.
     */
    void setLabel( const QString& v );

    /**
     * @return The string to be used for showing this item's current status.
     */
    const QString& status() const { return mStatus; }
    /**
     * Set the string to be used for showing this item's current status.
     * @param v The status string.
     */
    void setStatus( const QString& v );

    /**
     * @return Whether this item can be cancelled.
     */
    bool canBeCanceled() const { return mCanBeCanceled; }

    /**
     * @return Whether this item uses secure communication
     * (Account uses ssl, for example.).
     */
    bool usesCrypto() const { return mUsesCrypto; }

    /**
     * Set whether this item uses crypted communication, so listeners
     * can display a nice crypto icon.
     * @param v The value.
     */
    void setUsesCrypto( bool v );

    /**
     * @return The current progress value of this item in percent.
     */
    unsigned int progress() const { return mProgress; }

    /**
     * Set the progress (percentage of completion) value of this item.
     * @param v The percentage value.
     */
    void setProgress( unsigned int v );

    /**
     * Tell the item it has finished. This will emit progressItemCompleted()
     * result in the destruction of the item after all slots connected to this
     * signal have executed. This is the only way to get rid of an item and
     * needs to be called even if the item is cancelled. Don't use the item
     * after this has been called on it.
     */
    void setComplete();

    /**
     * Reset the progress value of this item to 0 and the status string to
     * the empty string.
     */
    void reset() { setProgress( 0 ); setStatus( QString::null ); mCompleted = 0; }

    void cancel();

    // Often needed values for calculating progress.
    void setTotalItems( unsigned int v ) { mTotal = v; }
    unsigned int totalItems() const;
    void setCompletedItems( unsigned int v ) { mCompleted = v; }
    void incCompletedItems( unsigned int v = 1 ) { mCompleted += v; }
    unsigned int completedItems() const;

    /**
     * Recalculate progress according to total/completed items and update.
     */
    void updateProgress() { setProgress( mTotal? mCompleted*100/mTotal: 0 ); };

    void addChild( ProgressItem *kiddo );
    void removeChild( ProgressItem *kiddo );

    bool canceled() const { return mCanceled; }

signals:
    /**
     * Emitted when a new ProgressItem is added.
     * @param The ProgressItem that was added.
     */
    void progressItemAdded( KPIM::ProgressItem* );
    /**
     * Emitted when the progress value of an item changes.
     * @param  The item which got a new value.
     * @param  The value, for convenience.
     */
    void progressItemProgress( KPIM::ProgressItem*, unsigned int );
    /**
     * Emitted when a progress item was completed. The item will be
     * deleted afterwards, so slots connected to this are the last
     * chance to work with this item.
     * @param The completed item.
     */
    void progressItemCompleted( KPIM::ProgressItem* );
    /**
     * Emitted when an item was cancelled. It will _not_ go away immediately,
     * only when the owner sets it complete, which will usually happen. Can be
     * used to visually indicate the cancelled status of an item. Should be used
     * by the owner of the item to make sure it is set completed even if it is
     * cancelled. There is a ProgressManager::slotStandardCancelHandler which
     * simply sets the item completed and can be used if no other work needs to
     * be done on cancel.
     * @param The canceled item;
     */
    void progressItemCanceled( KPIM::ProgressItem* );
    /**
     * Emitted when the status message of an item changed. Should be used by
     * progress dialogs to update the status message for an item.
     * @param  The updated item.
     * @param  The new message.
     */
    void progressItemStatus( KPIM::ProgressItem*, const QString& );
    /**
     * Emitted when the label of an item changed. Should be used by
     * progress dialogs to update the label of an item.
     * @param  The updated item.
     * @param  The new label.
     */
    void progressItemLabel( KPIM::ProgressItem*, const QString& );
    /**
     * Emitted when the crypto status of an item changed. Should be used by
     * progress dialogs to update the crypto indicator of an item.
     * @param  The updated item.
     * @param  The new state.
     */
    void progressItemUsesCrypto( KPIM::ProgressItem*, bool );


  protected:
    /* Only to be used by our good friend the ProgressManager */
    ProgressItem( ProgressItem* parent,
                             const QString& id,
                             const QString& label,
                             const QString& status,
                             bool isCancellable,
                             bool usesCrypto );
    virtual ~ProgressItem();


  private:
    QString mId;
    QString mLabel;
    QString mStatus;
    ProgressItem* mParent;
    bool mCanBeCanceled;
    unsigned int mProgress;
    ProgressItemMap mChildren;
    unsigned int mTotal;
    unsigned int mCompleted;
    bool mWaitingForKids;
    bool mCanceled;
    bool mUsesCrypto;
};

/**
 * The ProgressManager singleton keeps track of all ongoing transactions
 * and notifies observers (progress dialogs) when their progress percent value
 * changes, when they are completed (by their owner), and when they are canceled.
 * Each ProgressItem emits those signals individually and the singleton
 * broadcasts them. Use the ::createProgressItem() statics to acquire an item
 * and then call ->setProgress( int percent ) on it everytime you want to update
 * the item and ->setComplete() when the operation is done. This will delete the
 * item. Connect to the item's progressItemCanceled() signal to be notified when
 * the user cancels the transaction using one of the observing progress dialogs
 * or by calling item->cancel() in some other way. The owner is responsible for
 * calling setComplete() on the item, even if it is canceled. Use the
 * standardCancelHandler() slot if that is all you want to do on cancel.
 *
 * Note that if you request an item with a certain id and there is already
 * one with that id, there will not be a new one created but the existing
 * one will be returned. This is convenient for accessing items that are
 * needed regularly without the to store a pointer to them or to add child
 * items to parents by id.
 */

class ProgressManager : public QObject
{

  Q_OBJECT

  public:
    virtual ~ProgressManager();

    /**
     * @return The singleton instance of this class.
     */
    static ProgressManager * instance();

    /**
     * Use this to aquire a unique id number which can be used to discern
     * an operation from all others going on at the same time. Use that
     * number as the id string for your progressItem to ensure it is unique.
     * @return
     */
    static QString getUniqueID() { return QString::number( ++uID ); };

     /**
      * Creates a ProgressItem with a unique id and the given label.
      * This is the simplest way to aquire a progress item. It will not
      * have a parent and will be set to be cancellable and not using crypto.
      */
     static ProgressItem * createProgressItem( const QString &label ) {
       return instance()->createProgressItemImpl( 0, getUniqueID(), label,
                                                  QString::null, true, false );
     }

    /**
     * Creates a new progressItem with the given parent, id, label and initial
     * status.
     *
     * @param parent Specify an already existing item as the parent of this one.
     * @param id Used to identify this operation for cancel and progress info.
     * @param label The text to be displayed by progress handlers
     * @param status Additional text to be displayed for the item.
     * @param canBeCanceled can the user cancel this operation?
     * @param usesCrypto does the operation use secure transports (SSL)
     * Cancelling the parent will cancel the children as well (if they can be
     * cancelled) and ongoing children prevent parents from finishing.
     * @return The ProgressItem representing the operation.
     */
     static ProgressItem * createProgressItem( ProgressItem* parent,
                                               const QString& id,
                                               const QString& label,
                                               const QString& status = QString::null,
                                               bool canBeCanceled = true,
                                               bool usesCrypto = false ) {
       return instance()->createProgressItemImpl( parent, id, label, status,
                                                  canBeCanceled, usesCrypto );
     }

     /**
      * Use this version if you have the id string of the parent and want to
      * add a subjob to it.
      */
     static ProgressItem * createProgressItem( const QString& parent,
                                               const QString& id,
                                               const QString& label,
                                               const QString& status = QString::null,
                                               bool canBeCanceled = true,
                                               bool usesCrypto = false ) {
       return instance()->createProgressItemImpl( parent, id, label,
                                                 status, canBeCanceled, usesCrypto );
     }

     /**
      * Version without a parent.
      */
     static ProgressItem * createProgressItem( const QString& id,
                                               const QString& label,
                                               const QString& status = QString::null,
                                               bool canBeCanceled = true,
                                               bool usesCrypto = false ) {
       return instance()->createProgressItemImpl( 0, id, label, status,
                                                  canBeCanceled, usesCrypto );
     }


    /**
     * @return true when there is no more progress item
     */
    bool isEmpty() const { return mTransactions.isEmpty(); }

    /**
     * @return the only top level progressitem when there's only one.
     * Returns 0 if there is no item, or more than one top level item.
     */
    ProgressItem* singleItem() const;

    /**
     * Ask all listeners to show the progress dialog, because there is
     * something that wants to be shown.
     */
    static void emitShowProgressDialog() {
       instance()->emitShowProgressDialogImpl();
    }

  signals:
    /** @see ProgressItem::progressItemAdded() */
    void progressItemAdded( KPIM::ProgressItem* );
    /** @see ProgressItem::progressItemProgress() */
    void progressItemProgress( KPIM::ProgressItem*, unsigned int );
    /** @see ProgressItem::progressItemCompleted() */
    void progressItemCompleted( KPIM::ProgressItem* );
    /** @see ProgressItem::progressItemCanceled() */
    void progressItemCanceled( KPIM::ProgressItem* );
    /** @see ProgressItem::progressItemStatus() */
    void progressItemStatus( KPIM::ProgressItem*, const QString& );
    /** @see ProgressItem::progressItemLabel() */
    void progressItemLabel( KPIM::ProgressItem*, const QString& );
    /** @see ProgressItem::progressItemUsesCrypto() */
    void progressItemUsesCrypto( KPIM::ProgressItem*, bool );

    /**
     * Emitted when an operation requests the listeners to be shown.
     * Use emitShowProgressDialog() to trigger it.
     */
    void showProgressDialog();
  public slots:

    /**
     * Calls setCompleted() on the item, to make sure it goes away.
     * Provided for convenience.
     * @param item the canceled item.
     */
    void slotStandardCancelHandler( KPIM::ProgressItem* item );

    /**
     * Aborts all running jobs. Bound to "Esc"
     */
    void slotAbortAll();

  private slots:
    void slotTransactionCompleted( KPIM::ProgressItem *item );

  private:
    ProgressManager();
     // prevent unsolicited copies
    ProgressManager( const ProgressManager& );

    virtual ProgressItem* createProgressItemImpl(
                ProgressItem* parent, const QString& id,
                const QString& label, const QString& status,
                bool cancellable, bool usesCrypto );
    virtual ProgressItem* createProgressItemImpl(
                const QString& parent,  const QString& id,
                const QString& label, const QString& status,
                bool cancellable, bool usesCrypto );
    void emitShowProgressDialogImpl();

    QDict< ProgressItem > mTransactions;
    static ProgressManager *mInstance;
    static unsigned int uID;
};

}

#endif // __KPIM_PROGRESSMANAGER_H__
