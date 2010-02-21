/*
    Copyright (c) 2010 Grégory Oestreicher <greg@kamago.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "davcollectionsfetchjob.h"
#include "davutils.h"
#include "urlconfigurationdialog.h"

#include <QtGui/QStandardItem>
#include <QtGui/QStandardItemModel>

#include <QMessageBox>

UrlConfigurationDialog::UrlConfigurationDialog( QWidget *parent )
  : KDialog( parent )
{
  mUi.setupUi( mainWidget() );
  
  mModel = new QStandardItemModel();
  QStringList headers;
  headers << i18n( "Display name" ) << i18n( "URL" );
  mModel->setHorizontalHeaderLabels( headers );
  
  mUi.discoveredUrls->setModel( mModel );
  connect( mModel, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ),
           this, SLOT( onModelDataChanged( const QModelIndex &, const QModelIndex & ) ) );
  
  connect( mUi.remoteUrl, SIGNAL( textChanged( const QString & ) ), this, SLOT( checkUserInput() ) );
  connect( mUi.authenticationRequired, SIGNAL( toggled( bool ) ), this, SLOT( checkUserInput() ) );
  connect( mUi.username, SIGNAL( textChanged( const QString & ) ), this, SLOT( checkUserInput() ) );
  connect( mUi.password, SIGNAL( textChanged( const QString & ) ), this, SLOT( checkUserInput() ) );
  
  connect( mUi.fetchButton, SIGNAL( clicked() ), this, SLOT( onFetchButtonClicked() ) );
  connect( this, SIGNAL( okClicked() ), this, SLOT( onOkButtonClicked() ) );
  
  this->checkUserInput();
}

UrlConfigurationDialog::~UrlConfigurationDialog()
{
}

DavUtils::Protocol UrlConfigurationDialog::protocol() const
{
  return DavUtils::Protocol( mUi.remoteProtocol->selected() );
}

void UrlConfigurationDialog::setProtocol( DavUtils::Protocol p )
{
  mUi.remoteProtocol->setSelected( p );
}

QString UrlConfigurationDialog::remoteUrl() const
{
  return mUi.remoteUrl->text();
}

void UrlConfigurationDialog::setRemoteUrl( const QString &v )
{
  mUi.remoteUrl->setText( v );
}

bool UrlConfigurationDialog::authenticationRequired() const
{
  return mUi.authenticationRequired->isChecked();
}

void UrlConfigurationDialog::setAuthenticationRequired( bool b )
{
  mUi.authenticationRequired->setChecked( b );
}

bool UrlConfigurationDialog::useKWallet() const
{
  return mUi.useKWallet->isChecked();
}

void UrlConfigurationDialog::setUseKWallet( bool b )
{
  mUi.useKWallet->setChecked( b );
}

QString UrlConfigurationDialog::username() const
{
  return mUi.username->text();
}

void UrlConfigurationDialog::setUsername( const QString &v )
{
  mUi.username->setText( v );
}

QString UrlConfigurationDialog::password() const
{
  return mUi.password->text();
}

void UrlConfigurationDialog::setPassword( const QString &v )
{
  mUi.password->setText( v );
}

void UrlConfigurationDialog::checkUserInput()
{
  if( !mUi.remoteUrl->text().isEmpty() && this->checkUserAuthInput() ) {
    mUi.fetchButton->setEnabled( true );
    if( mModel->rowCount() > 0 )
      this->enableButtonOk( true );
  }
  else {
    mUi.fetchButton->setEnabled( false );
    this->enableButtonOk( false );
  }
}

void UrlConfigurationDialog::onFetchButtonClicked()
{
  for( int i = 0; i < mModel->rowCount(); ++i ) {
    mModel->removeRow( i );
  }
  
  KUrl url( mUi.remoteUrl->text() );
  if( this->authenticationRequired() ) {
    url.setUser( this->username() );
    url.setPassword( this->password() );
  }
  
  DavUtils::DavUrl davUrl( url, this->protocol() );
  DavCollectionsFetchJob *job = new DavCollectionsFetchJob( davUrl );
  connect( job, SIGNAL( result( KJob * ) ), this, SLOT( onCollectionsFetchDone( KJob * ) ) );
  job->start();
}

void UrlConfigurationDialog::onOkButtonClicked()
{
  if( !this->remoteUrl().endsWith( '/' ) ) {
    QString url = this->remoteUrl();
    url.append( QLatin1Char( '/' ) );
    this->setRemoteUrl( url );
  }
}

void UrlConfigurationDialog::onCollectionsFetchDone( KJob *job )
{
  if( job->error() )
    return;
  
  DavCollectionsFetchJob *davJob = qobject_cast<DavCollectionsFetchJob*>( job );
  
  DavCollection::List collections = davJob->collections();
  
  foreach( const DavCollection &collection, collections ) {
    this->addModelRow( collection.displayName(), collection.url() );
  }
  
  this->checkUserInput();
}

void UrlConfigurationDialog::onModelDataChanged( const QModelIndex &topLeft, const QModelIndex &bottomRight )
{
  // Actually only the display name can be changed, so no stricts checks are required
  QString newName = topLeft.data().toString();
  QString url = topLeft.sibling( topLeft.row(), 1 ).data().toString();
  QMessageBox box;
  box.setText( newName + " / " + url );
  box.exec();
}

bool UrlConfigurationDialog::checkUserAuthInput() {
  bool ret = false;
  
  if( !mUi.authenticationRequired->isChecked() ) {
    ret = true;
  }
  else {
    if( !mUi.username->text().isEmpty() && !mUi.password->text().isEmpty() )
      ret = true;
  }
  
  return ret;
}

void UrlConfigurationDialog::addModelRow( const QString &displayName, const QString &url )
{
  QStandardItem *rootItem = mModel->invisibleRootItem();
  QList<QStandardItem*> items;

  QStandardItem *displayNameStandardItem = new QStandardItem( displayName );
  displayNameStandardItem->setEditable( true );
  items << displayNameStandardItem;
  QStandardItem *urlStandardItem = new QStandardItem( url );
  urlStandardItem->setEditable( false );
  items << urlStandardItem;
  rootItem->appendRow( items );
}