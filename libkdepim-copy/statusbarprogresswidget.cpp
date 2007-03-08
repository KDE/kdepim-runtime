/*
  statusbarprogresswidget.cpp

  This file is part of KMail, the KDE mail client.

  (C) 2004 KMail Authors
  Includes StatusbarProgressWidget which is based on KIOLittleProgressDlg
  by Matt Koss <koss@miesto.sk>

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


#include "ssllabel.h"


#include <QMouseEvent>
#include <QHBoxLayout>
#include <QEvent>
#include <QProgressBar>

using KPIM::SSLLabel;
#include "progressmanager.h"
using KPIM::ProgressItem;
using KPIM::ProgressManager;

#include <kiconloader.h>
#include <kdebug.h>

#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QToolTip>
#include <klocale.h>
#include <QLayout>
#include <QStackedWidget>
#include <q3frame.h>

#include "progressdialog.h"
#include "statusbarprogresswidget.h"

using namespace KPIM;

//-----------------------------------------------------------------------------
StatusbarProgressWidget::StatusbarProgressWidget( ProgressDialog* progressDialog, QWidget* parent, bool button )
  : QFrame( parent ), mCurrentItem( 0 ), mProgressDialog( progressDialog ),
    mDelayTimer( 0 ), mBusyTimer( 0 )
{
  m_bShowButton = button;
  int w = fontMetrics().width( " 999.9 kB/s 00:00:01 " ) + 8;
  box = new QHBoxLayout( this );
  box->setMargin(0);
  box->setSpacing(0);

  m_pButton = new QPushButton( this );
  m_pButton->setSizePolicy( QSizePolicy( QSizePolicy::Minimum,
                                         QSizePolicy::Minimum ) );
  m_pButton->setIcon( SmallIcon( "go-up" ) );
  box->addWidget( m_pButton  );
  stack = new QStackedWidget( this );
  stack->setMaximumHeight( fontMetrics().height() );
  box->addWidget( stack );

  m_sslLabel = new SSLLabel( this );
  box->addWidget( m_sslLabel );

  m_pButton->setToolTip( i18n("Open detailed progress dialog") );

  m_pProgressBar = new QProgressBar( this );
#ifdef __GNUC__
#warning "kde4 porting\n";  
#endif
  //m_pProgressBar->setLineWidth( 1 );
  //m_pProgressBar->setFrameStyle( Q3Frame::Box );
  m_pProgressBar->installEventFilter( this );
  m_pProgressBar->setMinimumWidth( w );
  stack->insertWidget( 1,m_pProgressBar );

  m_pLabel = new QLabel( QString(), this );
  m_pLabel->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
  m_pLabel->installEventFilter( this );
  m_pLabel->setMinimumWidth( w );
  stack->insertWidget( 2, m_pLabel );
  m_pButton->setMaximumHeight( fontMetrics().height() );
  setMinimumWidth( minimumSizeHint().width() );

  mode = None;
  setMode();

  connect( m_pButton, SIGNAL( clicked() ),
           progressDialog, SLOT( slotToggleVisibility() ) );

  connect ( ProgressManager::instance(), SIGNAL( progressItemAdded( KPIM::ProgressItem * ) ),
            this, SLOT( slotProgressItemAdded( KPIM::ProgressItem * ) ) );
  connect ( ProgressManager::instance(), SIGNAL( progressItemCompleted( KPIM::ProgressItem * ) ),
            this, SLOT( slotProgressItemCompleted( KPIM::ProgressItem * ) ) );

  connect ( progressDialog, SIGNAL( visibilityChanged( bool )),
            this, SLOT( slotProgressDialogVisible( bool ) ) );

  mDelayTimer = new QTimer( this );
  connect ( mDelayTimer, SIGNAL( timeout() ),
            this, SLOT( slotShowItemDelayed() ) );
}

// There are three cases: no progressitem, one progressitem (connect to it directly),
// or many progressitems (display busy indicator). Let's call them 0,1,N.
// In slot..Added we can only end up in 1 or N.
// In slot..Removed we can end up in 0, 1, or we can stay in N if we were already.

void StatusbarProgressWidget::slotProgressItemAdded( ProgressItem *item )
{
  if ( item->parent() ) return; // we are only interested in top level items
  connectSingleItem(); // if going to 1 item
  if ( mCurrentItem ) { // Exactly one item
    delete mBusyTimer;
    mBusyTimer = 0;
    mDelayTimer->setSingleShot( true );
    mDelayTimer->start( 1000 );
  }
  else { // N items
    if ( !mBusyTimer ) {
      mBusyTimer = new QTimer( this );
      connect( mBusyTimer, SIGNAL( timeout() ),
               this, SLOT( slotBusyIndicator() ) );
      mDelayTimer->setSingleShot( true );
      mDelayTimer->start( 1000 );
    }
  }
}

void StatusbarProgressWidget::slotProgressItemCompleted( ProgressItem *item )
{
  if ( item->parent() ) return; // we are only interested in top level items
  connectSingleItem(); // if going back to 1 item
  if ( ProgressManager::instance()->isEmpty() ) { // No item
    // Done. In 5s the progress-widget will close, then we can clean up the statusbar
    QTimer::singleShot( 5000, this, SLOT( slotClean() ) );
  } else if ( mCurrentItem ) { // Exactly one item
    delete mBusyTimer;
    mBusyTimer = 0;
    activateSingleItemMode();
  }
}

void StatusbarProgressWidget::connectSingleItem()
{
  if ( mCurrentItem ) {
    disconnect ( mCurrentItem, SIGNAL( progressItemProgress( KPIM::ProgressItem *, unsigned int ) ),
                 this, SLOT( slotProgressItemProgress( KPIM::ProgressItem *, unsigned int ) ) );
    mCurrentItem = 0;
  }
  mCurrentItem = ProgressManager::instance()->singleItem();
  if ( mCurrentItem ) {
    connect ( mCurrentItem, SIGNAL( progressItemProgress( KPIM::ProgressItem *, unsigned int ) ),
              this, SLOT( slotProgressItemProgress( KPIM::ProgressItem *, unsigned int ) ) );
  }
}

void StatusbarProgressWidget::activateSingleItemMode()
{
  m_pProgressBar->setMaximum( 100 );
  m_pProgressBar->setValue( mCurrentItem->progress() );
  m_pProgressBar->setTextVisible( true );
}

void StatusbarProgressWidget::slotShowItemDelayed()
{
  bool noItems = ProgressManager::instance()->isEmpty();
  if ( mCurrentItem ) {
    activateSingleItemMode();
  } else if ( !noItems ) { // N items
    m_pProgressBar->setMaximum( 0 );
    m_pProgressBar->setTextVisible( false );
    Q_ASSERT( mBusyTimer );
    if ( mBusyTimer )
      mBusyTimer->start( 100 );
  }

  if ( !noItems && mode == None ) {
    mode = Progress;
    setMode();
  }
}

void StatusbarProgressWidget::slotBusyIndicator()
{
  int p = m_pProgressBar->value();
  m_pProgressBar->setValue( p + 10 );
}

void StatusbarProgressWidget::slotProgressItemProgress( ProgressItem *item, unsigned int value )
{
  Q_ASSERT( item == mCurrentItem); // the only one we should be connected to
  m_pProgressBar->setValue( value );
}

void StatusbarProgressWidget::slotSetSSL( bool ssl )
{
  m_sslLabel->setEncrypted( ssl );
}

void StatusbarProgressWidget::setMode() {
  switch ( mode ) {
  case None:
    if ( m_bShowButton ) {
      m_pButton->hide();
    }
    m_sslLabel->setState( SSLLabel::Done );
    // show the empty label in order to make the status bar look better
    stack->show();
    stack->setCurrentWidget( m_pLabel );
    break;

#if 0
  case Label:
    if ( m_bShowButton ) {
      m_pButton->show();
    }
    m_sslLabel->setState( m_sslLabel->lastState() );
    stack->show();
    stack->raiseWidget( m_pLabel );
    break;
#endif

  case Progress:
    stack->show();
    stack->setCurrentWidget( m_pProgressBar );
    if ( m_bShowButton ) {
      m_pButton->show();
    }
    m_sslLabel->setState( m_sslLabel->lastState() );
    break;
  }
}

void StatusbarProgressWidget::slotClean()
{
  // check if a new item showed up since we started the timer. If not, clear
  if ( ProgressManager::instance()->isEmpty() ) {
    m_pProgressBar->setValue( 0 );
    //m_pLabel->clear();
    mode = None;
    setMode();
  }
}

bool StatusbarProgressWidget::eventFilter( QObject *, QEvent *ev )
{
  if ( ev->type() == QEvent::MouseButtonPress ) {
    QMouseEvent *e = (QMouseEvent*)ev;

    if ( e->button() == Qt::LeftButton && mode != None ) {    // toggle view on left mouse button
      // Consensus seems to be that we should show/hide the fancy dialog when the user
      // clicks anywhere in the small one.
      mProgressDialog->slotToggleVisibility();
      return true;
    }
  }
  return false;
}

void StatusbarProgressWidget::slotProgressDialogVisible( bool b )
{
  // Update the hide/show button when the detailed one is shown/hidden
  if ( b ) {
    m_pButton->setIcon( SmallIcon( "go-down" ) );
    m_pButton->setToolTip( i18n("Hide detailed progress window") );
    setMode();
  } else {
    m_pButton->setIcon( SmallIcon( "go-up" ) );
    m_pButton->setToolTip( i18n("Show detailed progress window") );
  }
}

#include "statusbarprogresswidget.moc"
