/** -*- c++ -*-
 * progressdialog.cpp
 *
 *  Copyright (c) 2004 Till Adam <adam@kde.org>,
 *                     David Faure <faure@kde.org>
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

#include <QApplication>
#include <QLayout>
#include <QProgressBar>
#include <QTimer>
#include <q3header.h>
#include <QObject>
#include <q3scrollview.h>
#include <QToolButton>
#include <QPushButton>
#include <khbox.h>
#include <kvbox.h>

//Added by qt3to4:
#include <QCloseEvent>
#include <QEvent>
#include <QFrame>
#include <QLabel>

#include <klocale.h>
#include <kdialog.h>
#include <KStandardGuiItem>
#include <kiconloader.h>
#include <kdebug.h>

#include "progressdialog.h"
#include "progressmanager.h"
#include "ssllabel.h"


namespace KPIM {

class TransactionItem;

TransactionItemView::TransactionItemView( QWidget * parent,
                                          const char * name,
                                          Qt::WFlags f )
    : QScrollArea( parent ) {
  setFrameStyle( NoFrame );
  mBigBox = new KVBox( this );
  //mBigBox->setSpacing( 5 );
  setWidget( mBigBox );
  setWidgetResizable( true );
  //resize( 300, 200 );
  //setResizePolicy( Q3ScrollView::AutoOneFit ); // Fit so that the box expands horizontally
}

TransactionItem* TransactionItemView::addTransactionItem( ProgressItem* item, bool first )
{
   TransactionItem *ti = new TransactionItem( mBigBox, item, first );
   mBigBox->layout()->addWidget( ti );
   //ti->hide();
   //QTimer::singleShot( 1000, ti, SLOT( show() ) );
   
   resize( mBigBox->width(), mBigBox->height() );
   
   return ti;
}

// TODO
void TransactionItemView::resizeContents( int w, int h )
{
  //kDebug(5300) << k_funcinfo << w << "," << h << endl;
  //Q3ScrollView::resizeContents( w, h );
  // Tell the layout in the parent (progressdialog) that our size changed
  updateGeometry();
  // Resize the parent (progressdialog) - this works but resize horizontally too often
  //parentWidget()->adjustSize();

#ifdef QT3_SUPPORT
  QApplication::sendPostedEvents( 0, QEvent::ChildInserted );
  QApplication::sendPostedEvents( 0, QEvent::LayoutHint );
#endif
  QSize sz = parentWidget()->sizeHint();
  int currentWidth = parentWidget()->width();
  // Don't resize to sz.width() every time when it only reduces a little bit
  if ( currentWidth < sz.width() || currentWidth > sz.width() + 100 )
    currentWidth = sz.width();
  parentWidget()->resize( currentWidth, sz.height() );
}

QSize TransactionItemView::sizeHint() const
{
  return minimumSizeHint();
}

QSize TransactionItemView::minimumSizeHint() const
{
  int f = 2 * frameWidth();
  // Make room for a vertical scrollbar in all cases, to avoid a horizontal one
  int vsbExt = verticalScrollBar()->sizeHint().width();
  int minw = topLevelWidget()->width() / 3;
  int maxh = topLevelWidget()->height() / 2;
  QSize sz( mBigBox->minimumSizeHint() );
  sz.setWidth( qMax( sz.width(), minw ) + f + vsbExt );
  sz.setHeight( qMin( sz.height(), maxh ) + f );
  return sz;
}


void TransactionItemView::slotLayoutFirstItem()
{
  /*
     The below relies on some details in Qt's behaviour regarding deleting
     objects. This slot is called from the destroyed signal of an item just
     going away. That item is at that point still in the  list of chilren, but
     since the vtable is already gone, it will have type QObject. The first
     one with both the right name and the right class therefor is what will
     be the first item very shortly. That's the one we want to remove the
     hline for.
  */
  TransactionItem *ti = mBigBox->findChild<KPIM::TransactionItem*>( "TransactionItem" );
  if ( ti ) {
    ti->hideHLine();
  }
}


// ----------------------------------------------------------------------------

TransactionItem::TransactionItem( QWidget* parent,
                                  ProgressItem *item, bool first )
  : KVBox( parent ), mCancelButton( 0 ), mItem( item )

{
  setSpacing( 2 );
  setMargin( 2 );
  setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed ) );

  mFrame = new QFrame( this );
  mFrame->setFrameShape( QFrame::HLine );
  mFrame->setFrameShadow( QFrame::Raised );
  mFrame->show();
  setStretchFactor( mFrame, 3 );
  layout()->addWidget( mFrame );

  KHBox *h = new KHBox( this );
  h->setSpacing( 5 );
  layout()->addWidget( h );

  mItemLabel = new QLabel( item->label(), h );
  h->layout()->addWidget( mItemLabel );
  h->setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed ) );

  mProgress = new QProgressBar( h );
  mProgress->setMaximum(100);
  mProgress->setValue( item->progress() );
  h->layout()->addWidget( mProgress );

  if ( item->canBeCanceled() ) {
    mCancelButton = new QPushButton( SmallIcon( "cancel" ), QString(), h );
    mCancelButton->setToolTip( i18n("Cancel this operation.") );
    connect ( mCancelButton, SIGNAL( clicked() ),
              this, SLOT( slotItemCanceled() ));
    h->layout()->addWidget( mCancelButton );
  }
  
  h = new KHBox( this );
  h->setSpacing( 5 );
  h->setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed ) );
  layout()->addWidget( h );
  mSSLLabel = new SSLLabel( h );
  mSSLLabel->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
  h->layout()->addWidget( mSSLLabel );
  mItemStatus =  new QLabel( item->status(), h );
  h->layout()->addWidget( mItemStatus );
  setCrypto( item->usesCrypto() );
  if( first ) hideHLine();
}

TransactionItem::~TransactionItem()
{
}

void TransactionItem::hideHLine()
{
    mFrame->hide();
}

void TransactionItem::setProgress( int progress )
{
  mProgress->setValue( progress );
}

void TransactionItem::setLabel( const QString& label )
{
  mItemLabel->setText( label );
}

void TransactionItem::setStatus( const QString& status )
{
  mItemStatus->setText( status );
}

void TransactionItem::setCrypto( bool on )
{
  if (on)
    mSSLLabel->setEncrypted( true );
  else
    mSSLLabel->setEncrypted( false );

  mSSLLabel->setState( mSSLLabel->lastState() );
}

void TransactionItem::slotItemCanceled()
{
  if ( mItem )
    mItem->cancel();
}


void TransactionItem::addSubTransaction( ProgressItem* /*item*/ )
{

}


// ---------------------------------------------------------------------------

ProgressDialog::ProgressDialog( QWidget* alignWidget, QWidget* parent, const char* name )
    : OverlayWidget( alignWidget, parent, name ), mWasLastShown( false )
{
    setFrameStyle( QFrame::Panel | QFrame::Sunken ); // QFrame

    mScrollView = new TransactionItemView( this, "ProgressScrollView" );
    layout()->addWidget( mScrollView );

    // No more close button for now, since there is no more autoshow
    /*
    QVBox* rightBox = new QVBox( this );
    QToolButton* pbClose = new QToolButton( rightBox );
    pbClose->setAutoRaise(true);
    pbClose->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
    pbClose->setFixedSize( 16, 16 );
    pbClose->setIcon( KIconLoader::global()->loadIconSet( "window-close", K3Icon::Small, 14 ) );
    pbClose->setToolTip( i18n( "Hide detailed progress window" ) );
    connect(pbClose, SIGNAL(clicked()), this, SLOT(slotClose()));
    QWidget* spacer = new QWidget( rightBox ); // don't let the close button take up all the height
    rightBox->setStretchFactor( spacer, 100 );
    */

    /*
     * Get the singleton ProgressManager item which will inform us of
     * appearing and vanishing items.
     */
    ProgressManager *pm = ProgressManager::instance();
    connect ( pm, SIGNAL( progressItemAdded( KPIM::ProgressItem* ) ),
              this, SLOT( slotTransactionAdded( KPIM::ProgressItem* ) ) );
    connect ( pm, SIGNAL( progressItemCompleted( KPIM::ProgressItem* ) ),
              this, SLOT( slotTransactionCompleted( KPIM::ProgressItem* ) ) );
    connect ( pm, SIGNAL( progressItemProgress( KPIM::ProgressItem*, unsigned int ) ),
              this, SLOT( slotTransactionProgress( KPIM::ProgressItem*, unsigned int ) ) );
    connect ( pm, SIGNAL( progressItemStatus( KPIM::ProgressItem*, const QString& ) ),
              this, SLOT( slotTransactionStatus( KPIM::ProgressItem*, const QString& ) ) );
    connect ( pm, SIGNAL( progressItemLabel( KPIM::ProgressItem*, const QString& ) ),
              this, SLOT( slotTransactionLabel( KPIM::ProgressItem*, const QString& ) ) );
    connect ( pm, SIGNAL( progressItemUsesCrypto(KPIM::ProgressItem*, bool) ),
              this, SLOT( slotTransactionUsesCrypto( KPIM::ProgressItem*, bool ) ) );
    connect ( pm, SIGNAL( showProgressDialog() ),
              this, SLOT( slotShow() ) );
}

void ProgressDialog::closeEvent( QCloseEvent* e )
{
  e->accept();
  hide();
}


/*
 *  Destructor
 */
ProgressDialog::~ProgressDialog()
{
    // no need to delete child widgets.
}

void ProgressDialog::slotTransactionAdded( ProgressItem *item )
{
   TransactionItem *parent = 0;
   if ( item->parent() ) {
     if ( mTransactionsToListviewItems.contains( item->parent() ) ) {
       parent = mTransactionsToListviewItems[ item->parent() ];
       parent->addSubTransaction( item );
     }
   } else {
     const bool first = mTransactionsToListviewItems.empty();
     TransactionItem *ti = mScrollView->addTransactionItem( item, first );
     if ( ti )
       mTransactionsToListviewItems.insert( item, ti );
     if ( first && mWasLastShown )
       QTimer::singleShot( 1000, this, SLOT( slotShow() ) );

   }
}

void ProgressDialog::slotTransactionCompleted( ProgressItem *item )
{
   if ( mTransactionsToListviewItems.contains( item ) ) {
     TransactionItem *ti = mTransactionsToListviewItems[ item ];
     mTransactionsToListviewItems.remove( item );
     ti->setItemComplete();
     QTimer::singleShot( 3000, ti, SLOT( deleteLater() ) );
     // see the slot for comments as to why that works
     connect ( ti, SIGNAL( destroyed() ),
               mScrollView, SLOT( slotLayoutFirstItem() ) );
   }
   // This was the last item, hide.
   if ( mTransactionsToListviewItems.empty() )
     QTimer::singleShot( 3000, this, SLOT( slotHide() ) );
}

void ProgressDialog::slotTransactionCanceled( ProgressItem* )
{
}

void ProgressDialog::slotTransactionProgress( ProgressItem *item,
                                              unsigned int progress )
{
   if ( mTransactionsToListviewItems.contains( item ) ) {
     TransactionItem *ti = mTransactionsToListviewItems[ item ];
     ti->setProgress( progress );
   }
}

void ProgressDialog::slotTransactionStatus( ProgressItem *item,
                                            const QString& status )
{
   if ( mTransactionsToListviewItems.contains( item ) ) {
     TransactionItem *ti = mTransactionsToListviewItems[ item ];
     ti->setStatus( status );
   }
}

void ProgressDialog::slotTransactionLabel( ProgressItem *item,
                                           const QString& label )
{
   if ( mTransactionsToListviewItems.contains( item ) ) {
     TransactionItem *ti = mTransactionsToListviewItems[ item ];
     ti->setLabel( label );
   }
}


void ProgressDialog::slotTransactionUsesCrypto( ProgressItem *item,
                                                bool value )
{
   if ( mTransactionsToListviewItems.contains( item ) ) {
     TransactionItem *ti = mTransactionsToListviewItems[ item ];
     ti->setCrypto( value );
   }
}

void ProgressDialog::slotShow()
{
   setVisible( true );
}

void ProgressDialog::slotHide()
{
  // check if a new item showed up since we started the timer. If not, hide
  if ( mTransactionsToListviewItems.isEmpty() ) {
    setVisible( false );
  }
}

void ProgressDialog::slotClose()
{
  mWasLastShown = false;
  setVisible( false );
}

void ProgressDialog::setVisible( bool b )
{
  OverlayWidget::setVisible( b );
  emit visibilityChanged( b );
}

void ProgressDialog::slotToggleVisibility()
{
  /* Since we are only hiding with a timeout, there is a short period of
   * time where the last item is still visible, but clicking on it in
   * the statusbarwidget should not display the dialog, because there
   * are no items to be shown anymore. Guard against that.
   */
  mWasLastShown = isHidden();
  if ( !isHidden() || !mTransactionsToListviewItems.isEmpty() )
    setVisible( isHidden() );
}

}

#include "progressdialog.moc"
