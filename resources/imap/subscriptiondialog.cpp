/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "subscriptiondialog.h"

#include <QtCore/QCoreApplication>
#include <QtGui/QStandardItemModel>
#include <QtGui/QBoxLayout>
#include <QtGui/QKeyEvent>

#include <klocale.h>
#include <kdebug.h>
#include <klineedit.h>

#include <kimap/session.h>
#include <kimap/loginjob.h>
#include <kimap/unsubscribejob.h>
#include <kimap/subscribejob.h>

#include "imapaccount.h"
#include "sessionuiproxy.h"

#ifndef KDEPIM_MOBILE_UI
#include <QtGui/QHeaderView>
#include <QtGui/QCheckBox>
#include <QtGui/QLabel>
#include <QtGui/QTreeView>
#else
#include <QtGui/QListView>
#include <QtGui/QSortFilterProxyModel>
#include <kdescendantsproxymodel.h>

class CheckableFilterProxyModel : public QSortFilterProxyModel
{
public:
  CheckableFilterProxyModel( QObject *parent = 0 )
    : QSortFilterProxyModel( parent ) { }

protected:
  /*reimp*/ bool filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent ) const
  {
    QModelIndex sourceIndex = sourceModel()->index( sourceRow, 0, sourceParent );
    return sourceModel()->flags(sourceIndex) & Qt::ItemIsUserCheckable;
  }
};


#endif



SubscriptionDialog::SubscriptionDialog( QWidget *parent, SubscriptionDialog::SubscriptionDialogOptions option )
  : KDialog( parent ),
    m_session( 0 ),
    m_subscriptionChanged( false ),
    m_lineEdit( 0 ),
    m_filter( new SubscriptionFilterProxyModel( this ) ),
    m_model( new QStandardItemModel( this ) )
{
  setModal( true );
  setButtons( Ok | Cancel | User1 );

  setButtonText( User1, i18n( "Reload &List" ) );
  enableButton( User1, false );
  connect( this, SIGNAL(user1Clicked()),
          this, SLOT(onReloadRequested()) );

  QWidget *mainWidget = new QWidget( this );
  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainWidget->setLayout(mainLayout);
  setMainWidget( mainWidget );

  m_enableSubscription = new QCheckBox( i18n( "Enable server-side subscriptions" ) );
  mainLayout->addWidget(m_enableSubscription);

  QHBoxLayout *filterBarLayout = new QHBoxLayout;
  mainLayout->addLayout(filterBarLayout);

#ifndef KDEPIM_MOBILE_UI
  filterBarLayout->addWidget( new QLabel( i18n("Search:") ) );
#endif

  m_lineEdit = new KLineEdit( mainWidget );
  m_lineEdit->setClearButtonShown( true );
  connect( m_lineEdit, SIGNAL(textChanged(QString)),
           m_filter, SLOT(setSearchPattern(QString)) );
  filterBarLayout->addWidget( m_lineEdit );
  m_lineEdit->setFocus();

#ifndef KDEPIM_MOBILE_UI
  QCheckBox *checkBox = new QCheckBox( i18n("Subscribed only"), mainWidget );
  connect( checkBox, SIGNAL(stateChanged(int)),
           m_filter, SLOT(setIncludeCheckedOnly(int)) );
  filterBarLayout->addWidget( checkBox );

  QTreeView *treeView = new QTreeView( mainWidget );
  treeView->header()->hide();
  m_filter->setSourceModel( m_model );
  treeView->setModel( m_filter );
  mainLayout->addWidget( treeView );
#else
  m_lineEdit->hide();
  connect( m_lineEdit, SIGNAL(textChanged(QString)),
           this, SLOT(onMobileLineEditChanged(QString)) );

  QListView* listView = new QListView( mainWidget );

  KDescendantsProxyModel *flatModel = new KDescendantsProxyModel( listView );
  flatModel->setDisplayAncestorData( true );
  flatModel->setAncestorSeparator( "/" );
  flatModel->setSourceModel( m_model );

  CheckableFilterProxyModel *checkableModel = new CheckableFilterProxyModel( listView );
  checkableModel->setSourceModel( flatModel );

  m_filter->setSourceModel( checkableModel );

  listView->setModel( m_filter );
  mainLayout->addWidget( listView );

  // We want to get all the keyboard input all the time
  grabKeyboard();
#endif

  connect( m_model, SIGNAL(itemChanged(QStandardItem*)),
           this, SLOT(onItemChanged(QStandardItem*)) );

  if ( option & SubscriptionDialog::AllowToEnableSubscription ) {
#ifndef KDEPIM_MOBILE_UI
    connect( m_enableSubscription, SIGNAL( clicked ( bool ) ), treeView, SLOT( setEnabled(bool) ) );
#else
    connect( m_enableSubscription, SIGNAL( clicked ( bool ) ), listView, SLOT( setEnabled(bool) ) );
#endif
  } else {
    m_enableSubscription->hide();
  }
}

SubscriptionDialog::~SubscriptionDialog()
{
}

void SubscriptionDialog::setSubscriptionEnabled( bool enabled )
{
  m_enableSubscription->setChecked( enabled );
}

bool SubscriptionDialog::subscriptionEnabled() const
{
  return m_enableSubscription->isChecked();
}


void SubscriptionDialog::connectAccount( const ImapAccount &account, const QString &password )
{
  m_session = new KIMAP::Session( account.server(), account.port(), this );
  m_session->setUiProxy( SessionUiProxy::Ptr( new SessionUiProxy ) );

  KIMAP::LoginJob *login = new KIMAP::LoginJob( m_session );
  login->setUserName( account.userName() );
  login->setPassword( password );
  login->setEncryptionMode( account.encryptionMode() );
  login->setAuthenticationMode( account.authenticationMode() );

  connect( login, SIGNAL(result(KJob*)),
           this, SLOT(onLoginDone(KJob*)) );
  login->start();
}

bool SubscriptionDialog::isSubscriptionChanged() const
{
  return m_subscriptionChanged;
}

void SubscriptionDialog::onLoginDone( KJob *job )
{
  if ( !job->error() ) {
    onReloadRequested();
  }
}

void SubscriptionDialog::onReloadRequested()
{
  enableButton( User1, false );
  m_itemsMap.clear();
  m_model->clear();

  // we need a connection
  if ( !m_session
    || m_session->state() != KIMAP::Session::Authenticated ) {
    kWarning() <<"SubscriptionDialog - got no connection";
    enableButton( User1, true );
    return;
  }

  KIMAP::ListJob *list = new KIMAP::ListJob( m_session );
  list->setIncludeUnsubscribed( true );
  connect( list, SIGNAL(mailBoxesReceived(QList<KIMAP::MailBoxDescriptor>,QList<QList<QByteArray> >)),
           this, SLOT(onMailBoxesReceived(QList<KIMAP::MailBoxDescriptor>,QList<QList<QByteArray> >)) );
  connect( list, SIGNAL(result(KJob*)), this, SLOT(onFullListingDone(KJob*)) );
  list->start();
}

void SubscriptionDialog::onMailBoxesReceived( const QList<KIMAP::MailBoxDescriptor> &mailBoxes,
                                              const QList< QList<QByteArray> > &flags )
{
  for ( int i = 0; i<mailBoxes.size(); i++ ) {
    KIMAP::MailBoxDescriptor mailBox = mailBoxes[i];

    const QStringList pathParts = mailBox.name.split(mailBox.separator);
    const QString separator = mailBox.separator;
    Q_ASSERT( separator.size() == 1 ); // that's what the spec says

    QString parentPath;
    QString currentPath;

    for ( int j = 0; j < pathParts.size(); ++j ) {
      const bool isDummy = j != pathParts.size() - 1;
      const bool isCheckable = !isDummy && !flags[i].contains("\\noselect");

      const QString pathPart = pathParts.at( j );
      currentPath += separator + pathPart;

      if ( m_itemsMap.contains(currentPath) ) {
        if ( !isDummy ) {
          QStandardItem *item = m_itemsMap[currentPath];
          item->setCheckable( isCheckable );
        }

      } else if ( !parentPath.isEmpty() ) {
        Q_ASSERT( m_itemsMap.contains(parentPath) );

        QStandardItem *parentItem = m_itemsMap[parentPath];

        QStandardItem *item = new QStandardItem( pathPart );
        item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
        item->setCheckable( isCheckable );
        item->setData( currentPath.mid(1), PathRole );
        parentItem->appendRow( item );
        m_itemsMap[currentPath] = item;

      } else {
        QStandardItem *item = new QStandardItem( pathPart );
        item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
        item->setCheckable( isCheckable );
        item->setData( currentPath.mid(1), PathRole );
        m_model->appendRow( item );
        m_itemsMap[currentPath] = item;
      }

      parentPath = currentPath;
    }
  }
}

void SubscriptionDialog::onFullListingDone( KJob *job )
{
  if ( job->error() ) {
    enableButton( User1, true );
    return;
  }

  KIMAP::ListJob *list = new KIMAP::ListJob( m_session );
  list->setIncludeUnsubscribed( false );
  connect( list, SIGNAL(mailBoxesReceived(QList<KIMAP::MailBoxDescriptor>,QList<QList<QByteArray> >)),
           this, SLOT(onSubscribedMailBoxesReceived(QList<KIMAP::MailBoxDescriptor>,QList<QList<QByteArray> >)) );
  connect( list, SIGNAL(result(KJob*)), this, SLOT(onReloadDone(KJob*)) );
  list->start();
}

void SubscriptionDialog::onSubscribedMailBoxesReceived( const QList<KIMAP::MailBoxDescriptor> &mailBoxes,
                                                        const QList< QList<QByteArray> > &flags )
{
  Q_UNUSED( flags );
  for ( int i = 0; i<mailBoxes.size(); i++ ) {
    KIMAP::MailBoxDescriptor mailBox = mailBoxes[i];
    QString descriptor = mailBox.separator + mailBox.name;

    if ( m_itemsMap.contains( descriptor ) ) {
      QStandardItem *item = m_itemsMap[mailBox.separator + mailBox.name];
      item->setCheckState( Qt::Checked );
      item->setData( Qt::Checked, InitialStateRole );
    }
  }
}

void SubscriptionDialog::onReloadDone( KJob *job )
{
  Q_UNUSED( job );
  enableButton( User1, true );
}

void SubscriptionDialog::onItemChanged( QStandardItem *item )
{
  QFont font = item->font();
  font.setBold( item->checkState()!=item->data( InitialStateRole ).toInt() );
  item->setFont( font );
}

void SubscriptionDialog::slotButtonClicked( int button )
{
  if ( button == KDialog::Ok ) {
    applyChanges();
    accept();
  } else {
    KDialog::slotButtonClicked( button );
  }
}

void SubscriptionDialog::applyChanges()
{
  QList<QStandardItem*> items = m_itemsMap.values();

  while ( !items.isEmpty() ) {
    QStandardItem *item = items.takeFirst();

    if ( item->checkState()!=item->data( InitialStateRole ).toInt() ) {
      if ( item->checkState() == Qt::Checked ) {
        kDebug() << "Subscribing" << item->data( PathRole );
        KIMAP::SubscribeJob *subscribe = new KIMAP::SubscribeJob( m_session );
        subscribe->setMailBox( item->data( PathRole ).toString() );
        subscribe->exec();
      } else {
        kDebug() << "Unsubscribing" << item->data( PathRole );
        KIMAP::UnsubscribeJob *unsubscribe = new KIMAP::UnsubscribeJob( m_session );
        unsubscribe->setMailBox( item->data( PathRole ).toString() );
        unsubscribe->exec();
      }

      m_subscriptionChanged = true;
    }
  }
}

SubscriptionFilterProxyModel::SubscriptionFilterProxyModel( QObject* parent )
  : KRecursiveFilterProxyModel( parent ), m_checkedOnly( false )
{

}

void SubscriptionFilterProxyModel::setSearchPattern( const QString &pattern )
{
  m_pattern = pattern;
  invalidate();
}

void SubscriptionFilterProxyModel::setIncludeCheckedOnly( bool checkedOnly )
{
  m_checkedOnly = checkedOnly;
  invalidate();
}

void SubscriptionFilterProxyModel::setIncludeCheckedOnly( int checkedOnlyState )
{
  m_checkedOnly = (checkedOnlyState == Qt::Checked);
  invalidate();
}

bool SubscriptionFilterProxyModel::acceptRow(int sourceRow, const QModelIndex &sourceParent) const
{
  QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);

  const bool checked = sourceIndex.data(Qt::CheckStateRole).toInt()==Qt::Checked;

  if ( m_checkedOnly && !checked ) {
    return false;
  } else if ( !m_pattern.isEmpty() ) {
    const QString text = sourceIndex.data(Qt::DisplayRole).toString();
    return text.contains( m_pattern, Qt::CaseInsensitive );
  } else {
    return true;
  }
}

void SubscriptionDialog::onMobileLineEditChanged( const QString &text )
{
    if ( !text.isEmpty() && !m_lineEdit->isVisible() ) {
      m_lineEdit->show();
      m_lineEdit->setFocus();
      m_lineEdit->grabKeyboard(); // Now the line edit runs the show
    } else if ( text.isEmpty() && m_lineEdit->isVisible() ) {
      m_lineEdit->hide();
      grabKeyboard(); // Line edit gone, so let's get all the events for us again
    }
}

void SubscriptionDialog::keyPressEvent( QKeyEvent *event )
{
#ifndef KDEPIM_MOBILE_UI
  KDialog::keyPressEvent( event );
#else
  static bool isSendingEvent = false;

  if ( !isSendingEvent
    && !event->text().isEmpty()
    && !m_lineEdit->isVisible() ) {
    isSendingEvent = true;
    QCoreApplication::sendEvent( m_lineEdit, event );
    isSendingEvent = false;
  } else {
    KDialog::keyPressEvent( event );
  }
#endif
}

#include "subscriptiondialog.moc"
