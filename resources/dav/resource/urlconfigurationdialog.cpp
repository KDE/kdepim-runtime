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

#include "urlconfigurationdialog.h"

#include "davcollectionmodifyjob.h"
#include "davcollectionsfetchjob.h"
#include "davutils.h"
#include "settings.h"

#include <kmessagebox.h>

#include <QtGui/QStandardItem>
#include <QtGui/QStandardItemModel>

UrlConfigurationDialog::UrlConfigurationDialog( QWidget *parent )
  : KDialog( parent )
{
  mUi.setupUi( mainWidget() );
  mUi.credentialsGroup->setVisible( false );

  mModel = new QStandardItemModel();
  initModel();

  mUi.discoveredUrls->setModel( mModel );
  mUi.discoveredUrls->setRootIsDecorated( false );
  connect( mModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
           this, SLOT(onModelDataChanged(QModelIndex,QModelIndex)) );

  connect( mUi.remoteProtocol, SIGNAL(changed(int)), this, SLOT(onConfigChanged()) );
  connect( mUi.remoteUrl, SIGNAL(textChanged(QString)), this, SLOT(onConfigChanged()) );
  connect( mUi.useDefaultCreds, SIGNAL(toggled(bool)), this, SLOT(onConfigChanged()) );
  connect( mUi.username, SIGNAL(textChanged(QString)), this, SLOT(onConfigChanged()) );
  connect( mUi.password, SIGNAL(textChanged(QString)), this, SLOT(onConfigChanged()) );

  connect( mUi.fetchButton, SIGNAL(clicked()), this, SLOT(onFetchButtonClicked()) );
  connect( this, SIGNAL(okClicked()), this, SLOT(onOkButtonClicked()) );

  checkUserInput();
}

UrlConfigurationDialog::~UrlConfigurationDialog()
{
}

DavUtils::Protocol UrlConfigurationDialog::protocol() const
{
  return DavUtils::Protocol( mUi.remoteProtocol->selected() );
}

void UrlConfigurationDialog::setProtocol( DavUtils::Protocol protocol )
{
  mUi.remoteProtocol->setSelected( protocol );
}

QString UrlConfigurationDialog::remoteUrl() const
{
  return mUi.remoteUrl->text();
}

void UrlConfigurationDialog::setRemoteUrl( const QString &url )
{
  mUi.remoteUrl->setText( url );
}

bool UrlConfigurationDialog::useDefaultCredentials() const
{
  return mUi.useDefaultCreds->isChecked();
}

void UrlConfigurationDialog::setUseDefaultCredentials( bool defaultCreds )
{
  if ( defaultCreds )
    mUi.useDefaultCreds->setChecked( true );
  else
    mUi.useSpecificCreds->setChecked( true );
}

QString UrlConfigurationDialog::username() const
{
  return mUi.username->text();
}

void UrlConfigurationDialog::setUsername( const QString &userName )
{
  mUi.username->setText( userName );
}

QString UrlConfigurationDialog::password() const
{
  return mUi.password->text();
}

void UrlConfigurationDialog::setPassword(const QString& password)
{
  mUi.password->setText( password );
}

void UrlConfigurationDialog::onConfigChanged()
{
  initModel();
  mUi.fetchButton->setEnabled( false );
  enableButtonOk( false );
  checkUserInput();
}

void UrlConfigurationDialog::checkUserInput()
{
  if ( !mUi.remoteUrl->text().isEmpty() && checkUserAuthInput() ) {
    mUi.fetchButton->setEnabled( true );
    if ( mModel->rowCount() > 0 )
      enableButtonOk( true );
  } else {
    mUi.fetchButton->setEnabled( false );
    enableButtonOk( false );
  }
}

void UrlConfigurationDialog::onFetchButtonClicked()
{
  mUi.discoveredUrls->setEnabled( false );
  initModel();

  if ( !remoteUrl().endsWith( QLatin1Char( '/' ) ) )
    setRemoteUrl( remoteUrl() + QLatin1Char( '/' ) );


  KUrl url( mUi.remoteUrl->text() );
  url.setUser( username() );
  url.setPassword( password() );

  DavUtils::DavUrl davUrl( url, protocol() );
  DavCollectionsFetchJob *job = new DavCollectionsFetchJob( davUrl );
  connect( job, SIGNAL(result(KJob*)), this, SLOT(onCollectionsFetchDone(KJob*)) );
  job->start();
}

void UrlConfigurationDialog::onOkButtonClicked()
{
  if ( !remoteUrl().endsWith( QLatin1Char( '/' ) ) )
    setRemoteUrl( remoteUrl() + QLatin1Char( '/' ) );
}

void UrlConfigurationDialog::onCollectionsFetchDone( KJob *job )
{
  mUi.discoveredUrls->setEnabled( true );

  if ( job->error() ) {
    KMessageBox::error( this, job->errorText() );
    return;
  }

  DavCollectionsFetchJob *davJob = qobject_cast<DavCollectionsFetchJob*>( job );

  const DavCollection::List collections = davJob->collections();

  foreach ( const DavCollection &collection, collections )
    addModelRow( collection.displayName(), collection.url() );

  checkUserInput();
}

void UrlConfigurationDialog::onModelDataChanged( const QModelIndex &topLeft, const QModelIndex& )
{
  // Actually only the display name can be changed, so no stricts checks are required
  const QString newName = topLeft.data().toString();
  const QString url = topLeft.sibling( topLeft.row(), 1 ).data().toString();

  KUrl fullUrl( url );
  fullUrl.setUser( username() );
  fullUrl.setPassword( password() );

  DavUtils::DavUrl davUrl( fullUrl, protocol() );
  DavCollectionModifyJob *job = new DavCollectionModifyJob( davUrl );
  job->setProperty( "displayname", newName );
  connect( job, SIGNAL(result(KJob*)), this, SLOT(onChangeDisplayNameFinished(KJob*)) );
  job->start();
  mUi.discoveredUrls->setEnabled( false );
}

void UrlConfigurationDialog::onChangeDisplayNameFinished( KJob *job )
{
  if ( job->error() ) {
    KMessageBox::error( this, job->errorText() );
  }

  onFetchButtonClicked();
}

void UrlConfigurationDialog::initModel()
{
  mModel->clear();
  QStringList headers;
  headers << i18n( "Display name" ) << i18n( "URL" );
  mModel->setHorizontalHeaderLabels( headers );
}

bool UrlConfigurationDialog::checkUserAuthInput()
{
  return ( mUi.useDefaultCreds->isChecked() || !( mUi.username->text().isEmpty() || mUi.password->text().isEmpty() ) );
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
